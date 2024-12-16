#ifndef LIBSOKKETTER_H
#define LIBSOKKETTER_H

#pragma once

#include "export_definitions.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace sokketter
{
    /**
     * @brief structure containing version of sokketter library.
     */
    struct EXPORTED version_information
    {
    public:
        version_information(const uint8_t major, const uint8_t minor, const uint8_t micro, const uint8_t nano,
                std::string git_hash);

        [[nodiscard]] auto major() const noexcept -> uint8_t;
        [[nodiscard]] auto minor() const noexcept -> uint8_t;
        [[nodiscard]] auto micro() const noexcept -> uint8_t;
        [[nodiscard]] auto nano() const noexcept -> uint8_t;

        /**
         * @brief gets short hash of last Git commit
         * @return string
         */
        [[nodiscard]] auto git_hash() const noexcept -> std::string;

        /**
         * @brief creates string based on structure parameters
         * @return string in format major.minor.micro.nano
         */
        [[nodiscard]] auto to_string() const noexcept -> std::string;

    private:
        uint8_t m_major = 0;
        uint8_t m_minor = 0;
        uint8_t m_micro = 0;
        uint8_t m_nano = 0;

        /**
         * @brief states short hash of last Git commit
         */
        std::string m_git_hash = "";
    };

    /**
     * @brief gets current version of sokketter library.
     * @return version structure.
     */
    auto EXPORTED version() noexcept -> version_information;
} // namespace sokketter

#endif // LIBSOKKETTER_H
