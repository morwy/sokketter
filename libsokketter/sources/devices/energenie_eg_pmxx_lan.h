#ifndef ENERGENIE_EG_PMXX_LAN_H
#define ENERGENIE_EG_PMXX_LAN_H

#pragma once

#include <devices/energenie_eg_base.h>
#include <sokketter_core.h>

#include <curl/curl.h>
#include <spdlog/spdlog.h>

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

    auto power_socket(size_t index, bool is_toggled) -> bool override;
    auto socket_status(size_t index) -> bool override;

    static auto write_callback(char *data, size_t size, size_t count, void *user_data) -> size_t;
    static auto http_post(CURL *curl, const std::string &url, const std::string &fields,
        std::string &response) -> bool;
    static auto http_get(CURL *curl, const std::string &url, std::string &response) -> bool;

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
