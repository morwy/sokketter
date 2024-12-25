#ifndef LIBSOKKETTER_H
#define LIBSOKKETTER_H

#pragma once

#include "export_definitions.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace sokketter {
    /**
     * @brief structure containing version of sokketter library.
     */
    struct EXPORTED version_information
    {
    public:
        version_information(const uint8_t major, const uint8_t minor, const uint8_t micro,
            const uint8_t nano, std::string git_hash);

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

    /**
     * @brief The settings_information class
     */
    struct EXPORTED settings_information
    {
        std::filesystem::path database_path;
        std::string database_password = "";
    };

    /**
     * @brief settings
     * @return
     */
    auto EXPORTED settings() noexcept -> settings_information;

    /**
     * @brief The socket class
     */
    class EXPORTED socket
    {
        const std::string name();
        const std::string description();

        bool toggle(const bool &is_enabled);
        bool status();
    };

    enum class power_strip_type
    {
        UNKNOWN = 0,
        ENERGENIE_PMx_x = 1
    };

    /**
     * @brief The device_filter class
     */
    struct EXPORTED power_strip_configuration
    {
        power_strip_type type = power_strip_type::UNKNOWN;
        std::string name = "";
        std::string description = "";
        std::string address = "";
    };

    /**
     * @brief The power_strip class
     */
    class EXPORTED power_strip
    {
    public:
        power_strip() = default;

        // const power_strip_configuration &configuration();
        // void configure(const power_strip_configuration &configuration);

        // bool is_connected() const;

        // const std::vector<socket> &sockets();
    };

    /**
     * @brief The device_filter class
     */
    struct EXPORTED device_filter
    {
        bool allow_disconnected_devices = true;
        std::vector<power_strip_type> allowed_types = {power_strip_type::ENERGENIE_PMx_x};
    };

    /**
     * @brief returns the list of power strip devices built depending on the provided filtering
     * settings.
     * @param filter
     * @return vector of power_strip objects
     */
    const std::vector<power_strip> EXPORTED devices(const device_filter &filter = {});
} // namespace sokketter

#endif // LIBSOKKETTER_H
