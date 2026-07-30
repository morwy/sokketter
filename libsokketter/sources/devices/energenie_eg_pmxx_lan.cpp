#include "energenie_eg_pmxx_lan.h"

#include <curl/curl.h>

#include <sstream>
#include <string>
#include <vector>

energenie_eg_pmxx_lan::energenie_eg_pmxx_lan()
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "{}: constructed object {}.", __FUNCTION__, static_cast<void *>(this));

    sokketter::power_strip_configuration configuration;
    configuration.type = sokketter::power_strip_type::ENERGENIE_EG_PMXX_LAN;
    this->configure(configuration);

    /**
     * Configure sockets.
     */
    m_socket_number = 4;

    for (size_t socket_index = 1; socket_index < m_socket_number + 1; socket_index++)
    {
        sokketter::socket socket(socket_index,
            std::bind(&energenie_eg_pmxx_lan::power_socket, this, std::placeholders::_1,
                std::placeholders::_2),
            std::bind(&energenie_eg_pmxx_lan::socket_status, this, std::placeholders::_1));
        m_sockets.push_back(socket);
    }
}

energenie_eg_pmxx_lan::~energenie_eg_pmxx_lan()
{
    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: destructed object {}.", this->to_string(),
        static_cast<void *>(this));
}

auto energenie_eg_pmxx_lan::initialize(std::shared_ptr<kommpot::device_communication> communication)
    -> bool
{
    const auto &identification_variant = communication->identification();
    const auto *identification =
        std::get_if<kommpot::ethernet_device_identification>(&identification_variant);
    if (identification == nullptr)
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Provided identification is not Ethernet.");
        return false;
    }

    if (!power_strip_base::initialize(communication))
    {
        return false;
    }

    auto configuration = this->configuration();

    m_serial_number = identification->mac;
    configuration.id = identification->mac;
    configuration.address = identification->ip;

    this->configure(configuration);

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: initialization.", this->to_string());

    return true;
}

auto energenie_eg_pmxx_lan::identification() -> const kommpot::ethernet_device_identification
{
    kommpot::ethernet_device_identification identification;

    identification.ip = "*";
    identification.port = 5000;
    identification.mac = "88:B6:27:*";
    identification.protocol = kommpot::ethernet_protocol_type::TCP;

    return identification;
}

auto energenie_eg_pmxx_lan::power_socket(size_t index, bool is_toggled) -> bool
{
    if (m_communication == nullptr)
    {
        SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER,
            "{}: skipping powering socket due to disconnected status.", this->to_string(), index);
        return false;
    }

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: powering socket {} {}.", this->to_string(), index,
        is_toggled ? "on" : "off");

    const std::string &address = this->configuration().address;

    CURL *curl = create_session();
    if (curl == nullptr)
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed to initialize the HTTP session.", this->to_string());
        return false;
    }

    std::string response;
    bool success = login(curl, address, m_password, response);
    if (success)
    {
        const std::string fields = "cte" + std::to_string(index) + "=" + (is_toggled ? "1" : "0");
        success = http_post(curl, "http://" + address + "/", fields, response);
    }

    logout(curl, address);

    curl_easy_cleanup(curl);

    if (!success)
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed powering socket {}.", this->to_string(), index);
        return false;
    }

    /**
     * The device echoes the full socket states in its response, so refresh the cache from it and
     * avoid a follow-up status query. Fall back to the requested state if parsing yields nothing.
     */
    if (!update_states_from_response(response) && index >= 1 && index <= m_socket_states.size())
    {
        m_socket_states[index - 1] = is_toggled;
        m_socket_states_time = std::chrono::steady_clock::now();
    }

    return true;
}

auto energenie_eg_pmxx_lan::socket_status(size_t index) -> bool
{
    if (m_communication == nullptr)
    {
        SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER,
            "{}: skipping checking socket status due to disconnected status.", this->to_string(),
            index);
        return false;
    }

    const bool cache_fresh =
        m_socket_states_valid &&
        (std::chrono::steady_clock::now() - m_socket_states_time) < SOCKET_STATES_CACHE_TTL;

    if (!cache_fresh)
    {
        SPDLOG_LOGGER_DEBUG(
            SOKKETTER_LOGGER, "{}: checking socket {} status.", this->to_string(), index);

        if (!refresh_socket_states())
        {
            SPDLOG_LOGGER_ERROR(
                SOKKETTER_LOGGER, "{}: failed reading socket {} status.", this->to_string(), index);
            return false;
        }
    }

    if (index < 1 || index > m_socket_states.size())
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: socket {} status is not available.", this->to_string(), index);
        return false;
    }

    return m_socket_states[index - 1];
}

auto energenie_eg_pmxx_lan::refresh_socket_states() -> bool
{
    const std::string &address = this->configuration().address;

    CURL *curl = create_session();
    if (curl == nullptr)
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed to initialize the HTTP session.", this->to_string());
        return false;
    }

    std::string response;
    const bool success = login(curl, address, m_password, response);

    logout(curl, address);

    curl_easy_cleanup(curl);

    if (!success)
    {
        return false;
    }

    return update_states_from_response(response);
}

auto energenie_eg_pmxx_lan::update_states_from_response(const std::string &body) -> bool
{
    const std::vector<bool> states = parse_socket_states(body);
    if (states.empty())
    {
        return false;
    }

    m_socket_states = states;
    m_socket_states_time = std::chrono::steady_clock::now();
    m_socket_states_valid = true;

    return true;
}

auto energenie_eg_pmxx_lan::write_callback(char *data, size_t size, size_t count, void *user_data)
    -> size_t
{
    const size_t length = size * count;
    auto *buffer = static_cast<std::string *>(user_data);
    buffer->append(data, length);
    return length;
}

auto energenie_eg_pmxx_lan::http_post(
    CURL *curl, const std::string &url, const std::string &fields, std::string &response) -> bool
{
    response.clear();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    return curl_easy_perform(curl) == CURLE_OK;
}

auto energenie_eg_pmxx_lan::http_get(CURL *curl, const std::string &url, std::string &response)
    -> bool
{
    response.clear();

    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    return curl_easy_perform(curl) == CURLE_OK;
}

auto energenie_eg_pmxx_lan::create_session() -> CURL *
{
    CURL *curl = curl_easy_init();
    if (curl == nullptr)
    {
        return nullptr;
    }

    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, HTTP_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTP_TIMEOUT_SECONDS);

    return curl;
}

auto energenie_eg_pmxx_lan::login(CURL *curl, const std::string &address,
    const std::string &password, std::string &response) -> bool
{
    return http_post(curl, "http://" + address + "/login.html", "pw=" + password, response);
}

auto energenie_eg_pmxx_lan::logout(CURL *curl, const std::string &address) -> void
{
    std::string response;
    http_get(curl, "http://" + address + "/login.html", response);
}

auto energenie_eg_pmxx_lan::parse_socket_states(const std::string &body) -> std::vector<bool>
{
    std::vector<bool> states;

    const std::string marker = "sockstates = ";
    const auto marker_position = body.find(marker);
    if (marker_position == std::string::npos)
    {
        return states;
    }

    const auto open_bracket = body.find('[', marker_position);
    const auto close_bracket = body.find(']', open_bracket);
    if (open_bracket == std::string::npos || close_bracket == std::string::npos)
    {
        return states;
    }

    const std::string list = body.substr(open_bracket + 1, close_bracket - open_bracket - 1);

    std::istringstream stream(list);
    std::string token;
    while (std::getline(stream, token, ','))
    {
        const auto first = token.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            continue;
        }
        const auto last = token.find_last_not_of(" \t\r\n");
        states.push_back(token.substr(first, last - first + 1) == "1");
    }

    return states;
}
