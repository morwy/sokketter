#ifndef SOKKETTER_CORE_H
#define SOKKETTER_CORE_H

#pragma once

#include <database_storage.h>
#include <libsokketter.h>
#include <spdlog/logger.h>
#include <third-party/kommpot/libkommpot/include/libkommpot.h>
#include <update_check_storage.h>

#include <atomic>
#include <mutex>
#include <set>
#include <string>
#include <thread>

constexpr auto LOGGER_NAME = "sokketter";
#define SOKKETTER_LOGGER spdlog::get(LOGGER_NAME)

class sokketter_core
{
public:
    static auto instance() -> sokketter_core &
    {
        static sokketter_core instance;
        return instance;
    }

    sokketter_core(const sokketter_core &) = delete;
    auto operator=(const sokketter_core &) -> void = delete;

    auto initialize() -> bool;
    auto deinitialize() -> bool;

    auto settings() noexcept -> sokketter::settings_structure;
    auto set_settings(const sokketter::settings_structure &settings) noexcept -> void;

    auto database() -> database_storage &;

    auto devices(const sokketter::device_filter &filter = {})
        -> const std::vector<std::shared_ptr<sokketter::power_strip>> &;

    auto devices(const sokketter::device_filter &filter, sokketter::device_callback device_cb,
        sokketter::status_callback status_cb) -> void;

    auto device(const size_t &index) -> std::shared_ptr<sokketter::power_strip>;

    auto device(const std::string &serial_number) -> std::shared_ptr<sokketter::power_strip>;

    auto release_link() -> std::string;
    auto is_new_release_available(std::string &latest_version) -> bool;

    auto check_for_update_async() -> void;
    auto last_update_check_status() -> sokketter::update_check_status;

private:
    inline static constexpr auto RELEASE_LINK =
        "https://github.com/morwy/sokketter/releases/latest";
    inline static constexpr auto RELEASE_API_HOST = "api.github.com";
    inline static constexpr auto RELEASE_API_PATH = "/repos/morwy/sokketter/releases/latest";

    inline static constexpr uint32_t UPDATE_CHECK_TIMEOUT_MSECS = 10000;

    /**
     * @brief size of the buffer used for draining a response out of the communication.
     */
    inline static constexpr size_t RESPONSE_CHUNK_SIZE_BYTES = 4096;

    sokketter::settings_structure m_settings;
    std::shared_ptr<spdlog::logger> m_logger = nullptr;
    database_storage m_database;

    update_check_storage m_update_check_storage;
    std::mutex m_update_check_storage_mutex;
    std::mutex m_update_check_thread_mutex;
    std::thread m_update_check_thread;
    std::atomic_bool m_update_check_running = false;

    sokketter::device_callback m_device_cb = nullptr;
    sokketter::status_callback m_status_cb = nullptr;

    /**
     * @brief worker thread running the blocking device enumeration (last known address probe and
     * the kommpot search), so that the calling thread (e.g. the Qt GUI thread) returns
     * immediately.
     *
     * The search thread exits as soon as the asynchronous kommpot search is scheduled, while the
     * enumeration itself keeps running on a kommpot thread; therefore m_search_running covers the
     * whole enumeration and is only cleared when kommpot reports completion.
     */
    std::mutex m_search_thread_mutex;
    std::thread m_search_thread;
    std::atomic_bool m_search_running = false;

    sokketter_core() = default;
    ~sokketter_core() = default;

    auto initialize_logger() -> void;
    auto deinitialize_logger() -> void;

    static auto normalize_version_string(std::string version) -> std::string;
    static auto parse_version_parts(const std::string &version) -> std::vector<uint32_t>;

    auto is_newer_version(const std::string &current_version, const std::string &candidate_version)
        -> bool;

    auto logging_callback(const kommpot::callback_response_structure &response) -> void;

    /**
     * @brief probes the last known addresses of the stored network devices before the regular
     * network search.
     *
     * Every communication that answers is associated with a stored device by identity (MAC), so a
     * device is only re-attached when its MAC address still matches. A host with a foreign
     * identity at a probed address is ignored here; the regular search picks it up if it is a
     * supported device, while the stored device itself is searched for again. A stored device that
     * answers at an address different from its stored one (moved device or kommpot address
     * normalization) adopts the new address.
     *
     * @param filter stating which device types are being enumerated.
     * @param handled_ids receives the identities re-attached during the probe.
     * @return true when the database must be saved (a device adopted a new address), false
     * otherwise.
     */
    auto probe_last_known_addresses(
        const sokketter::device_filter &filter, std::set<std::string> &handled_ids) -> bool;

    /**
     * @brief processes the communications reported by an enumeration.
     *
     * @param communications the communications found by the enumeration.
     * @param handled_ids the identities re-attached via their last known address during this
     * enumeration run, used to skip the duplicate communication of the regular search. Owned by
     * the enumeration run (not by the object), so that the set of a finished run cannot race
     * with the one of a new run.
     */
    auto new_devices_received(
        std::vector<std::shared_ptr<kommpot::device_communication>> communications,
        const std::set<std::string> &handled_ids) -> void;

    auto new_status_received(kommpot::enumeration_status status) -> void;
};

#endif // CORE_H
