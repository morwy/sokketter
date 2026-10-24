#ifndef SOKKETTER_CORE_H
#define SOKKETTER_CORE_H

#pragma once

#include <database_storage.h>
#include <libsokketter.h>
#include <spdlog/logger.h>
#include <third-party/kommpot/libkommpot/include/libkommpot.h>
#include <update_check_storage.h>

#include <atomic>
#include <condition_variable>
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

    /**
     * @brief persists the device database to the storage. Takes the database mutex, so it is
     * safe to call from any thread.
     */
    auto save_database() -> void;

    /**
     * @brief removes a device from the database and persists the storage. Takes the database
     * mutex, so it is safe to call from any thread.
     */
    auto remove_device(std::shared_ptr<sokketter::power_strip> &device) -> void;

    /**
     * @brief blocking enumeration: probes last known addresses, runs a full kommpot search and
     * integrates the results into the database, exactly like the asynchronous path but executed
     * on the caller's thread. The database mutex is never held across blocking network or device
     * I/O; the probe and the device creation/reattachment run against snapshots and the database
     * mutations take the mutex only briefly.
     * @return the whole database sorted by name, as a copy of the device pointers that stays
     * valid independently of any concurrent enumeration.
     */
    auto devices(const sokketter::device_filter &filter = {})
        -> std::vector<std::shared_ptr<sokketter::power_strip>>;

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

    /**
     * @brief upper bound on the time the search thread waits for the asynchronous kommpot search
     * of a run to report completion. Acts only as a last-resort safety net, so that an
     * enumeration that never reports completion cannot wedge all later enumerations.
     */
    inline static constexpr uint32_t ENUMERATION_COMPLETION_TIMEOUT_MSECS = 300000;

    sokketter::settings_structure m_settings;
    std::shared_ptr<spdlog::logger> m_logger = nullptr;
    database_storage m_database;

    /**
     * @brief guards m_database. Every access to the device vector inside it, whether a lookup
     * from a caller thread or a mutation performed during an enumeration, must hold this
     * mutex, so that push_back/sort performed on an enumeration thread cannot race with a
     * concurrent iteration (e.g. the UI thread resolving a device by serial number on a
     * click).
     *
     * The private helper add_new_device and the direct m_database.load()/save() calls expect
     * the mutex to already be held by their caller; the public entry points and the enumeration
     * integration points acquire it themselves. The mutex is never held while acquiring any
     * other lock of this class and never held across blocking device I/O (create_and_reattach
     * reaches the stored devices through a snapshot instead), and lookups return copies of the
     * shared pointers under the lock, so callers keep the device objects alive through the
     * shared pointers.
     */
    std::mutex m_database_mutex;

    update_check_storage m_update_check_storage;
    std::mutex m_update_check_storage_mutex;
    std::mutex m_update_check_thread_mutex;
    std::thread m_update_check_thread;
    std::atomic_bool m_update_check_running = false;

    /**
     * @brief a single device enumeration request: the filter plus the callbacks bound to the run
     * itself, so that a new request can never overwrite (or race with) the callbacks of a run
     * that is still delivering results.
     */
    struct enumeration_request
    {
        sokketter::device_filter filter;
        sokketter::device_callback device_cb;
        sokketter::status_callback status_cb;
    };

    /**
     * @brief worker thread running the blocking device enumeration (last known address probe and
     * the kommpot search), so that the calling thread (e.g. the Qt GUI thread) returns
     * immediately.
     *
     * The search thread stays alive for the whole enumeration: once the asynchronous kommpot
     * search is scheduled, it blocks until kommpot reports completion, therefore
     * m_search_running accurately means that an enumeration is running from start to finish, and
     * is cleared only when the search thread has finished its last enumeration.
     */
    std::mutex m_search_thread_mutex;
    std::thread m_search_thread;
    std::atomic_bool m_search_running = false;

    /**
     * @brief debounce slot for a request that arrived while an enumeration was already running.
     * The latest request wins and is executed exactly once when the running enumeration
     * completes, so that a refresh request is never silently swallowed. Guarded by
     * m_enumeration_pending_mutex, which the search thread also acquires between enumerations to
     * consume a pending request; both clearing m_search_running and queueing a request hold this
     * mutex, so that a request arriving right at the end of an enumeration cannot be lost.
     */
    bool m_has_pending_request = false;
    std::shared_ptr<const enumeration_request> m_pending_request;
    std::mutex m_enumeration_pending_mutex;

    /**
     * @brief completion signal of the asynchronous kommpot search of the current enumeration run.
     * The search thread blocks on m_enumeration_completion_cv until the current run's status
     * callback reports COMPLETED, so that it cannot finish (and clear m_search_running) while
     * kommpot is still enumerating.
     *
     * Completions are tagged with a generation: every run claims a new m_enumeration_generation
     * before scheduling its kommpot search, and that search's COMPLETED callback records the
     * generation into m_completed_generation. A late completion from a run whose wait was
     * abandoned (by the timeout or a shutdown abort) is therefore inert: it cannot make a later
     * run believe its own enumeration finished when that later run's kommpot search was rejected
     * by kommpot's overlapping-enumeration guard and never reports completion of its own.
     */
    std::mutex m_enumeration_completion_mutex;
    std::condition_variable m_enumeration_completion_cv;
    uint64_t m_enumeration_generation = 0;
    uint64_t m_completed_generation = 0;

    /**
     * @brief set by deinitialize() to make the search thread give up waiting for a kommpot
     * completion that would never arrive, so that joining it during shutdown cannot hang.
     */
    std::atomic_bool m_search_thread_abort = false;

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
     * @brief runs one full enumeration synchronously on the search thread: the blocking USB
     * enumeration, the last known address probe, and the asynchronous kommpot ethernet search,
     * whose completion is waited for, so that this function returns only when the whole
     * enumeration is really over.
     *
     * @param request the request (filter plus run callbacks) to execute. Held by shared pointer,
     * so that the run context outlives the kommpot callbacks even when the search thread returns
     * early during a shutdown abort.
     */
    auto run_enumeration(const std::shared_ptr<const enumeration_request> &request) -> void;

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
     * @param database snapshot of the stored devices to build the probes from.
     * @param filter stating which device types are being enumerated.
     * @param handled_ids receives the identities re-attached during the probe.
     * @return true when the database must be saved (a device adopted a new address), false
     * otherwise.
     * @attention the probe does not structurally modify the database, so it runs without
     * holding m_database_mutex; the snapshot passed to it must be taken under that mutex.
     */
    auto probe_last_known_addresses(
        const std::vector<std::shared_ptr<sokketter::power_strip>> &database,
        const sokketter::device_filter &filter, std::set<std::string> &handled_ids) -> bool;

    /**
     * @brief turns the communications reported by an enumeration into devices without touching
     * the structure of the database: a fresh device is created through the factory (which opens
     * the communication and performs the device I/O), a communication of a device that was
     * already reattached at its last known address during this enumeration run is skipped, and
     * the communication of an already stored device reattaches that stored device (adopting the
     * new last known address when it moved). The devices that were not stored before are
     * collected in new_devices for the caller to integrate under the database mutex.
     *
     * @param communications the communications found by the enumeration.
     * @param handled_ids the identities re-attached via their last known address during this
     * enumeration run.
     * @param new_devices receives the freshly created devices that are not yet in the database.
     * @return true when a reattached stored device adopted a new address, meaning the database
     * must be saved, false otherwise.
     * @attention performs blocking device I/O, so it must not be called while holding
     * m_database_mutex: it takes the mutex only briefly to snapshot the stored devices, and each
     * device object serializes access to its own mutable state through its state mutex.
     */
    auto create_and_reattach(
        const std::vector<std::shared_ptr<kommpot::device_communication>> &communications,
        const std::set<std::string> &handled_ids,
        std::vector<std::shared_ptr<sokketter::power_strip>> &new_devices) -> bool;

    /**
     * @brief appends a freshly created device to the database when its identity is not present
     * yet, without persisting it; the caller saves the database at most once per enumeration run.
     *
     * @param device a device created by create_and_reattach.
     * @return true when the device was appended, false when it was already present (integrated
     * concurrently by another enumeration run).
     * @attention expects m_database_mutex to already be held by the caller; performs no I/O.
     */
    auto add_new_device(std::shared_ptr<sokketter::power_strip> device) -> bool;

    /**
     * @brief processes the communications reported by an enumeration.
     *
     * @param communications the communications found by the enumeration.
     * @param handled_ids the identities re-attached via their last known address during this
     * enumeration run, used to skip the duplicate communication of the regular search. Owned by
     * the enumeration run (not by the object), so that the set of a finished run cannot race
     * with the one of a new run.
     * @param device_cb the callback bound to the enumeration run that received the
     * communications, notified with the updated database.
     */
    auto new_devices_received(
        std::vector<std::shared_ptr<kommpot::device_communication>> communications,
        const std::set<std::string> &handled_ids,
        const sokketter::device_callback &device_cb) -> void;

    /**
     * @brief processes a status reported by an enumeration.
     *
     * @param status the status reported by kommpot.
     * @param status_cb the callback bound to the enumeration run the status belongs to.
     */
    auto new_status_received(
        kommpot::enumeration_status status, const sokketter::status_callback &status_cb) -> void;
};

#endif // SOKKETTER_CORE_H
