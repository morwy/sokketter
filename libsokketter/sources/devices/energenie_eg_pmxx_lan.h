#ifndef ENERGENIE_EG_PMXX_LAN_H
#define ENERGENIE_EG_PMXX_LAN_H

#pragma once

#include <devices/energenie_eg_base.h>
#include <sokketter_core.h>

#include <curl/curl.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <string>
#include <vector>

class energenie_eg_pmxx_lan : public energenie_eg_base
{
public:
    energenie_eg_pmxx_lan();
    ~energenie_eg_pmxx_lan();

    auto initialize(std::shared_ptr<kommpot::device_communication> communication) -> bool override;

    static auto identification() -> const kommpot::ethernet_device_identification;

private:
    std::string m_password = "1";

    /**
     * @brief maximum time in seconds allowed for connecting to and communicating with the device.
     */
    static constexpr long HTTP_TIMEOUT_SECONDS = 5;

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

    auto power_socket(size_t index, bool is_toggled) -> bool override;
    auto socket_status(size_t index) -> bool override;

    static auto write_callback(char *data, size_t size, size_t count, void *user_data) -> size_t;
    static auto http_post(CURL *curl, const std::string &url, const std::string &fields,
        std::string &response) -> bool;
    static auto http_get(CURL *curl, const std::string &url, std::string &response) -> bool;

    /**
     * @brief performs a single status query and refreshes the cached socket states.
     */
    auto refresh_socket_states() -> bool;

    /**
     * @brief refreshes the cached socket states from a page containing the "sockstates" list.
     * @return true if states were found and cached, false otherwise.
     */
    auto update_states_from_response(const std::string &body) -> bool;

    /**
     * @brief creates a new session handle with an in-memory cookie engine enabled.
     *
     * The device keeps the authenticated session in a cookie, so login, switching and logout must
     * all share the same handle.
     */
    static auto create_session() -> CURL *;

    static auto login(CURL *curl, const std::string &address, const std::string &password,
        std::string &response) -> bool;
    static auto logout(CURL *curl, const std::string &address) -> void;

    /**
     * @brief extracts the socket states from the "sockstates = [x,x,x,x]" declaration of the status
     * page.
     */
    static auto parse_socket_states(const std::string &body) -> std::vector<bool>;
};

#endif // ENERGENIE_EG_PMXX_LAN_H
