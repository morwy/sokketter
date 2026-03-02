#include "energenie_eg_pmxx_lan.h"

/**
 * @attention
 * @link
 */

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
    logout();

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

auto energenie_eg_pmxx_lan::connect_if_not_yet() -> bool
{
    if (!m_communication->is_open())
    {
        if (!m_communication->open())
        {
            SPDLOG_LOGGER_ERROR(
                SOKKETTER_LOGGER, "{}: failed to open communication.", this->to_string());
            return false;
        }
    }

    char end_session_character = END_SESSION_CHARACTER;

    /**
     * Old session may be still alive. Try several times to close it forcibly.
     */
    bool is_connected = false;
    constexpr size_t max_retries = 4;
    for (size_t retry = 1; retry < max_retries; retry++)
    {
        is_connected =
            m_communication->write({}, &end_session_character, sizeof(end_session_character));
        if (is_connected)
        {
            break;
        }
    }

    return is_connected;
}

auto energenie_eg_pmxx_lan::disconnect() -> void
{
    if (m_communication->is_open())
    {
        m_communication->close();
    }
}

auto energenie_eg_pmxx_lan::login(const std::string &password) -> bool
{
    connect_if_not_yet();

    /**
     * Read out current task.
     */
    auth_question_array question = {0};

    bool result = m_communication->read({}, question.data(), question.size());
    if (!result)
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed to read out current task.", this->to_string());
        return false;
    }

    auth_answer_structure answer(question, password);

    result = m_communication->write({}, &answer, sizeof(answer));
    if (!result)
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed to write authentication answer.", this->to_string());
        return false;
    }

    m_session.password = password;
    m_session.question = question;

    return true;
}

auto energenie_eg_pmxx_lan::logout() -> void
{
    if (m_communication != nullptr)
    {
        char end_session_character = END_SESSION_CHARACTER;

        const bool result =
            m_communication->write({}, &end_session_character, sizeof(end_session_character));
        if (!result)
        {
            SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "{}: failed to end session.", this->to_string());
        }
    }

    disconnect();
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

    if (!m_session.is_authenticated())
    {
        if (!login(m_password))
        {
            SPDLOG_LOGGER_ERROR(
                SOKKETTER_LOGGER, "{}: failed to login to device.", this->to_string());
            return false;
        }
    }

    encrypted_status_array status_array = m_status_array;

    const uint8_t value = is_toggled ? 0x1 : 0x2;
    status_array[3 - (index - 1)] =
        (((value ^ m_session.question[2]) + m_session.question[3]) ^ m_session.password.at(0)) +
        m_session.password.at(1);

    const bool result = m_communication->write({}, status_array.data(), status_array.size());
    if (!result)
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed to write socket {} status.", this->to_string(), index);
    }

    m_status_array = {0};

    return result;
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

    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "{}: checking socket {} status.", this->to_string(), index);

    if (!m_session.is_authenticated())
    {
        if (!login(m_password))
        {
            SPDLOG_LOGGER_ERROR(
                SOKKETTER_LOGGER, "{}: failed to login to device.", this->to_string());
            return false;
        }
    }

    if (m_status_array[0] == 0 && m_status_array[1] == 0 && m_status_array[2] == 0 &&
        m_status_array[3] == 0)
    {
        encrypted_status_array status_array;

        const bool result = m_communication->read({}, status_array.data(), status_array.size());
        if (!result)
        {
            SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "{}: failed to read out socket {} status.",
                this->to_string(), index);
            return false;
        }

        m_status_array = status_array;
    }

    const uint8_t status =
        (((m_status_array[3 - (index - 1)] - m_session.password.at(1)) ^ m_session.password.at(0)) -
            m_session.question[3]) ^
        m_session.question[2];

    switch (status)
    {
    case 0x22: /** protocol version 2.0 */
    case 0x82: /** protocol version 2.1 */
    case 0x92: /** protocol version WLAN */
    {
        return false;
    }
    case 0x11: /** protocol version 2.0 */
    case 0x41: /** protocol version 2.1 */
    case 0x51: /** protocol version WLAN */
    {
        return true;
    }
    default: {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "{}: received unknown status 0x{:02X} for socket {}.",
            this->to_string(), status, index);
        return false;
    }
    }
}
