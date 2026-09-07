#include "energenie_eg_pmxx_lan.h"

#include <array>
#include <sstream>
#include <string>
#include <vector>

energenie_eg_pmxx_lan::energenie_eg_pmxx_lan()
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "{}: constructed object {}.", __FUNCTION__, static_cast<void *>(this));

    m_configuration.type = sokketter::power_strip_type::ENERGENIE_EG_PMXX_LAN;
    m_configuration.authentication.type = sokketter::power_strip_authentication_type::PASSWORD_ONLY;

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
    if (SOKKETTER_LOGGER == nullptr)
    {
        return;
    }

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: destructed object {}.", this->to_string(),
        static_cast<void *>(this));
}

auto energenie_eg_pmxx_lan::initialize(std::shared_ptr<kommpot::device_communication> communication)
    -> bool
{
    if (communication == nullptr)
    {
        return false;
    }

    const std::lock_guard<std::mutex> lock(m_communication_mutex);

    const auto &identification_variant = communication->identification();
    const auto *identification =
        std::get_if<kommpot::http_device_identification>(&identification_variant);
    if (identification == nullptr)
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Provided identification is not HTTP.");
        return false;
    }

    if (!power_strip_base::initialize(communication))
    {
        return false;
    }

    kommpot::http_device_configuration configuration;
    configuration.timeout_ms = HTTP_TIMEOUT_MSECS;
    communication->set_configuration(configuration);

    m_serial_number = identification->mac;

    m_configuration.id = identification->mac;
    m_configuration.address = identification->address;

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: initialization.", this->to_string());

    return true;
}

auto energenie_eg_pmxx_lan::reconnect() -> bool
{
    const auto &address = m_configuration.address;
    if (address.empty())
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "{}: no address configured!", this->to_string());
        return false;
    }

    auto device_identification = identification();
    device_identification.address = address;

    if (address.find("://") != std::string::npos)
    {
        device_identification.port = 0;
    }

    auto communication = kommpot::device(device_identification);
    if (communication == nullptr)
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed creating the HTTP communication!", this->to_string());
        return false;
    }

    return initialize(communication);
}

auto energenie_eg_pmxx_lan::try_authenticate() -> bool
{
    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: trying to authenticate.", this->to_string());

    const std::lock_guard<std::mutex> lock(m_communication_mutex);

    if (m_communication == nullptr)
    {
        return false;
    }

    if (!m_communication->open())
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed to open the HTTP session.", this->to_string());
        return false;
    }

    std::string response = "";
    const bool is_logged_in = login(m_configuration.authentication.password, response);

    logout();

    m_communication->close();

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: authentication: {}.", this->to_string(),
        is_logged_in ? "success" : "failure");

    return is_logged_in;
}

auto energenie_eg_pmxx_lan::identification() -> const kommpot::http_device_identification
{
    kommpot::http_device_identification identification;

    identification.address = "*";
    identification.port = 80;
    identification.mac = "88:B6:27:*";

    return identification;
}

auto energenie_eg_pmxx_lan::power_socket(size_t index, bool is_toggled) -> bool
{
    const std::lock_guard<std::mutex> lock(m_communication_mutex);

    if (m_communication == nullptr)
    {
        SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER,
            "{}: skipping powering socket due to disconnected status.", this->to_string(), index);
        return false;
    }

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: powering socket {} {}.", this->to_string(), index,
        is_toggled ? "on" : "off");

    if (m_configuration.authentication.password.empty())
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "{}: no password provided for powering socket {}.",
            this->to_string(), index);
        return false;
    }

    if (!m_communication->open())
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed to open the HTTP session.", this->to_string());
        return false;
    }

    std::string response = "";
    bool result = login(m_configuration.authentication.password, response);
    if (result)
    {
        const std::string body = "cte" + std::to_string(index) + "=" + (is_toggled ? "1" : "0");
        result = request(kommpot::http_transfer_type::POST, "/", body, response);
    }

    logout();

    m_communication->close();

    if (!result)
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
    const std::lock_guard<std::mutex> lock(m_communication_mutex);

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
    if (m_configuration.authentication.password.empty())
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "{}: no password provided for reading socket states.",
            this->to_string());
        return false;
    }

    if (!m_communication->open())
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed to open the HTTP session.", this->to_string());
        return false;
    }

    std::string response = "";
    const bool is_logged_in = login(m_configuration.authentication.password, response);

    logout();

    m_communication->close();

    if (!is_logged_in)
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

auto energenie_eg_pmxx_lan::request(const kommpot::http_transfer_type &type,
    const std::string &resource_path, const std::string &body, std::string &response) -> bool
{
    response.clear();

    kommpot::http_transfer_configuration http_configuration;
    http_configuration.type = type;
    http_configuration.resource_path = resource_path;
    http_configuration.body = body;

    if (!body.empty())
    {
        http_configuration.content_type = "application/x-www-form-urlencoded";
    }

    kommpot::transfer_configuration configuration = http_configuration;

    if (!m_communication->write(configuration, nullptr, 0))
    {
        return false;
    }

    const auto *performed_configuration =
        std::get_if<kommpot::http_transfer_configuration>(&configuration);
    if (performed_configuration == nullptr)
    {
        return false;
    }

    std::array<char, RESPONSE_CHUNK_SIZE_BYTES> chunk = {};
    while (m_communication->read(configuration, chunk.data(), chunk.size()))
    {
        response.append(chunk.data(), performed_configuration->bytes_read);
    }

    return true;
}

auto energenie_eg_pmxx_lan::login(const std::string &password, std::string &response) -> bool
{
    if (!request(kommpot::http_transfer_type::POST, "/login.html", "pw=" + password, response))
    {
        return false;
    }

    const std::string marker = "action=\"/login.html\"";
    const auto marker_position = response.find(marker);
    if (marker_position != std::string::npos)
    {
        /**
         * Authentication failed if login form is present in the response.
         */
        return false;
    }

    return true;
}

auto energenie_eg_pmxx_lan::logout() -> void
{
    std::string response = "";
    request(kommpot::http_transfer_type::GET, "/login.html", "", response);
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
