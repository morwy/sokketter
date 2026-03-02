#ifndef ENERGENIE_EG_PMXX_LAN_H
#define ENERGENIE_EG_PMXX_LAN_H

#pragma once

#include <devices/energenie_eg_base.h>
#include <sokketter_core.h>

#include <spdlog/spdlog.h>

#include <array>

#if defined(__linux__) || defined(__CYGWIN__)
#    include <endian.h>
#elif defined(__APPLE__)
#    include <libkern/OSByteOrder.h>
#    define htole16(x) OSSwapHostToLittleInt16(x)
#elif defined(_WIN32)
#    if BYTE_ORDER == LITTLE_ENDIAN
#        define htole16(x) (x)
#    elif BYTE_ORDER == BIG_ENDIAN
#        define htole16(x) __builtin_bswap16(x)
#    endif
#endif

constexpr uint8_t END_SESSION_CHARACTER = '\x11';

/**
 * @attention maximum password length is limited to 8 characters by the device.
 */
constexpr size_t MAX_PASSWORD_SIZE_BYTES = 8;

struct auth_password_structure
{
    /**
     * @attention unused characters are filled with spaces (0x20).
     */
    std::array<uint8_t, MAX_PASSWORD_SIZE_BYTES> value;

    auth_password_structure()
    {
        value.fill(' ');
    }

    auth_password_structure(const std::string &password)
    {
        value.fill(' ');

        memcpy(value.data(), password.data(), std::min(password.size(), MAX_PASSWORD_SIZE_BYTES));

        if (password.size() > MAX_PASSWORD_SIZE_BYTES)
        {
            SPDLOG_LOGGER_WARN(SOKKETTER_LOGGER,
                "Provided password is too long ({} characters). It will be truncated to {} "
                "characters.",
                password.size(), MAX_PASSWORD_SIZE_BYTES);
        }
    }

    auto at(size_t index) const -> uint8_t
    {
        if (index >= MAX_PASSWORD_SIZE_BYTES)
        {
            throw std::out_of_range("Password index out of range.");
        }

        return value[index];
    }

    auto is_valid() const -> bool
    {
        for (const auto &byte : value)
        {
            if (byte != ' ')
            {
                return true;
            }
        }

        return false;
    }
};

constexpr size_t MAX_QUESTION_SIZE_BYTES = 4;

typedef std::array<uint8_t, MAX_QUESTION_SIZE_BYTES> auth_question_array;

#pragma pack(push, 1)
struct auth_answer_structure
{
    /**
     * @attention lower part is sent first, then higher part because of little-endian format.
     */
    uint16_t lower_part = 0;
    uint16_t higher_part = 0;

    auth_answer_structure(
        const auth_question_array &question, const auth_password_structure &password)
    {
        lower_part =
            ((uint16_t(question[0]) ^ uint16_t(password.at(2))) * uint16_t(password.at(0))) ^
            (uint16_t(password.at(6)) | (uint16_t(password.at(4)) << 8)) ^ uint16_t(question[2]);
        lower_part = htole16(lower_part);

        higher_part =
            ((uint16_t(question[1]) ^ uint16_t(password.at(3))) * uint16_t(password.at(1))) ^
            (uint16_t(password.at(7)) | (uint16_t(password.at(5)) << 8)) ^ uint16_t(question[3]);
        higher_part = htole16(higher_part);
    }
};
#pragma pack(pop)

constexpr size_t MAX_STATUS_SIZE_BYTES = 4;

typedef std::array<uint8_t, MAX_STATUS_SIZE_BYTES> encrypted_status_array;

struct auth_session_structure
{
    auth_password_structure password;
    auth_question_array question;

    auto is_authenticated() const -> bool
    {
        return password.is_valid() && question[0] != 0 && question[1] != 0 && question[2] != 0 &&
               question[3] != 0;
    }
};

class energenie_eg_pmxx_lan : public energenie_eg_base
{
public:
    energenie_eg_pmxx_lan();
    ~energenie_eg_pmxx_lan();

    auto initialize(std::shared_ptr<kommpot::device_communication> communication) -> bool override;

    static auto identification() -> const kommpot::ethernet_device_identification;

private:
    std::string m_password = "1";
    auth_session_structure m_session;
    encrypted_status_array m_status_array = {0};

    auto connect_if_not_yet() -> bool;
    auto disconnect() -> void;

    auto login(const std::string &password) -> bool;
    auto logout() -> void;

    auto power_socket(size_t index, bool is_toggled) -> bool override;
    auto socket_status(size_t index) -> bool override;
};

#endif // ENERGENIE_EG_PMXX_LAN_H
