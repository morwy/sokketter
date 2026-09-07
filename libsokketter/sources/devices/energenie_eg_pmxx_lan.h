#ifndef ENERGENIE_EG_PMXX_LAN_H
#define ENERGENIE_EG_PMXX_LAN_H

#pragma once

#include <devices/energenie_eg_base.h>
#include <sokketter_core.h>

#include <spdlog/spdlog.h>

#include <chrono>
#include <mutex>
#include <string>
#include <vector>

class energenie_eg_pmxx_lan : public energenie_eg_base
{
public:
    energenie_eg_pmxx_lan();
    ~energenie_eg_pmxx_lan();

    auto initialize(std::shared_ptr<kommpot::device_communication> communication) -> bool override;

    auto reconnect() -> bool override;

    [[nodiscard]] auto try_authenticate() -> bool override;

    static auto identification() -> const kommpot::http_device_identification;

private:
    /**
     * @brief maximum time in milliseconds allowed for connecting to and communicating with the
     * device.
     */
    static constexpr uint32_t HTTP_TIMEOUT_MSECS = 5000;

    /**
     * @brief size of the buffer used for draining a response out of the communication.
     */
    static constexpr size_t RESPONSE_CHUNK_SIZE_BYTES = 4096;

    /**
     * @brief how long cached socket states stay valid before another status query is issued.
     *
     * A single status query returns the states of every socket, so a short cache collapses the
     * burst of per-socket reads done when a device page is opened into one network round-trip.
     */
    static constexpr std::chrono::milliseconds SOCKET_STATES_CACHE_TTL{1000};

    std::vector<bool> m_socket_states;
    std::chrono::steady_clock::time_point m_socket_states_time{};
    bool m_socket_states_valid = false;

    /**
     * @brief serializes the session, the device keeps a single authenticated session at a time and
     * both the UI and the enumeration thread reach the device.
     */
    std::mutex m_communication_mutex;

    auto power_socket(size_t index, bool is_toggled) -> bool override;
    auto socket_status(size_t index) -> bool override;

    /**
     * @brief performs a single status query and refreshes the cached socket states.
     *
     * Caller must hold @ref m_communication_mutex.
     */
    auto refresh_socket_states() -> bool;

    /**
     * @brief refreshes the cached socket states from a page containing the "sockstates" list.
     * @return true if states were found and cached, false otherwise.
     */
    auto update_states_from_response(const std::string &body) -> bool;

    /**
     * @brief performs a request and drains its response body.
     */
    auto request(const kommpot::http_transfer_type &type, const std::string &resource_path,
        const std::string &body, std::string &response) -> bool;

    auto login(const std::string &password, std::string &response) -> bool;
    auto logout() -> void;

    /**
     * @brief extracts the socket states from the "sockstates = [x,x,x,x]" declaration of the status
     * page.
     */
    static auto parse_socket_states(const std::string &body) -> std::vector<bool>;
};

#endif // ENERGENIE_EG_PMXX_LAN_H
