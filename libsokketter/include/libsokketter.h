#ifndef LIBSOKKETTER_H
#define LIBSOKKETTER_H

#pragma once

#include "export_definitions.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
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
     * @brief structure containing settings of sokketter library.
     */
    struct EXPORTED settings_information
    {
        std::filesystem::path database_path;
        std::string database_password = "";
    };

    /**
     * @brief gets current settings of sokketter library.
     * @return settings structure.
     */
    auto EXPORTED settings() noexcept -> settings_information;

    /**
     * @brief structure containing configuration parameters of the specific socket.
     */
    struct EXPORTED socket_configuration
    {
        std::string id = "";
        std::string name = "Unnamed socket";
        std::string description = "";
    };

    /**
     * @brief the class for controlling and configuring the socket.
     */
    class EXPORTED socket
    {
    public:
        socket(const size_t index, std::function<bool(size_t, bool)> power_cb,
            std::function<bool(size_t)> status_cb);

        /**
         * @brief gets current configuration of the socket.
         * @return socket configuration structure.
         */
        [[nodiscard]] auto configuration() const noexcept -> const socket_configuration &;

        /**
         * @brief sets a new configuration of the socket.
         * @param configuration of the socket.
         */
        auto configure(const socket_configuration &configuration) -> void;

        /**
         * @brief powers on or off the socket based on the provided parameter.
         * @param on specifies to which state socket should be switched.
         * @return true in case of success, false in case of any failure.
         */
        auto power(const bool &on) const noexcept -> bool;

        /**
         * @brief toggles the socket state.
         * @return true in case of success, false in case of any failure.
         */
        auto toggle() const noexcept -> bool;

        /**
         * @brief gets current socket state.
         * @return true if powered on, false if powered off.
         */
        [[nodiscard]] auto is_powered_on() const noexcept -> bool;

        /**
         * @brief creates string based on socket status and its parameters.
         * @return string in format "SOCKET_NAME, status: STATUS".
         */
        [[nodiscard]] auto to_string() const noexcept -> std::string;

    private:
        socket_configuration m_configuration;

        size_t m_index = 0;
        std::function<bool(size_t, bool)> m_power_cb;
        std::function<bool(size_t)> m_status_cb;
    };

    /**
     * @brief the enum specifying supported devices.
     */
    enum class power_strip_type
    {
        UNKNOWN = 0,
        TEST_DEVICE = 1,
        GEMBIRD_MSIS_PM = 2,
        GEMBIRD_SIS_PM = 3,
        GEMBIRD_MSIS_PM_2 = 4,
        ENERGENIE_EG_PMS = 5,
        ENERGENIE_EG_PMS2 = 6,
    };

    /**
     * @brief converts power_strip_type to a readable string value.
     * @param type of power strip.
     * @return string.
     */
    auto EXPORTED power_strip_type_to_string(const power_strip_type &type) -> std::string;

    /**
     * @brief structure containing configuration parameters of the specific power strip.
     */
    struct EXPORTED power_strip_configuration
    {
        power_strip_type type = power_strip_type::UNKNOWN;
        std::string id = "";
        std::string name = "Unnamed power strip";
        std::string description = "";
        std::string address = "";
    };

    /**
     * @brief the class for controlling and configuring the power strip.
     */
    class EXPORTED power_strip
    {
    public:
        power_strip() = default;
        virtual ~power_strip() = default;

        /**
         * @warning states class is non-copyable.
         */
        power_strip(const power_strip &obj) = delete;
        auto operator=(const power_strip &obj) -> power_strip & = delete;

        /**
         * @warning states class is non-movable.
         */
        power_strip(power_strip &&obj) = delete;
        auto operator=(power_strip &&obj) -> power_strip & = delete;

        /**
         * @brief gets current configuration of the power strip.
         * @return power strip configuration structure.
         */
        [[nodiscard]] auto configuration() const noexcept -> const power_strip_configuration &;

        /**
         * @brief sets a new configuration of the power strip.
         * @param configuration of the power strip.
         */
        auto configure(const power_strip_configuration &configuration) -> void;

        /**
         * @brief gets connection stateo of the power strip.
         * @return bool if device is connected, false if device is not connected.
         */
        [[nodiscard]] virtual auto is_connected() const -> bool = 0;

        /**
         * @brief gets list of sockets controlled by the power strip.
         * @return vector of socket objects.
         */
        [[nodiscard]] virtual auto sockets() -> const std::vector<socket> & = 0;

        /**
         * @brief gets a specific socket controlled by the power strip.
         * @param index of the socket.
         * @return socket object as std::optional in case of success, std::nullopt in case of
         * failure.
         */
        [[nodiscard]] virtual auto socket(const size_t &index)
            -> const std::optional<std::reference_wrapper<socket>> = 0;

        /**
         * @brief creates string based on power strip parameters
         * @return string in format "POWER_STRIP_NAME (TYPE, ID, located at ADDRESS)".
         */
        [[nodiscard]] auto to_string() const noexcept -> std::string;

    private:
        power_strip_configuration m_configuration;
    };

    /**
     * @brief structure containing filtering settings for device gathering.
     */
    struct EXPORTED device_filter
    {
        bool allow_disconnected_devices = true;
        std::vector<power_strip_type> allowed_types = {};
    };

    /**
     * @brief returns the list of power strip devices built upon provided filtering settings.
     * @param filter settings stating which devices to list.
     * @return vector of power_strip objects.
     */
    auto EXPORTED devices(const device_filter &filter = {})
        -> const std::vector<std::unique_ptr<sokketter::power_strip>>;

    /**
     * @brief returns the power strip device by its index.
     * @param index of the device.
     * @return unique pointer to power_strip object or nullptr in case of any failure.
     */
    auto EXPORTED device(const size_t &index) -> const std::unique_ptr<sokketter::power_strip>;

    /**
     * @brief returns the power strip device by its serial number.
     * @param serial_number of the device.
     * @return unique pointer to power_strip object or nullptr in case of any failure.
     */
    auto EXPORTED device(const std::string &serial_number)
        -> const std::unique_ptr<sokketter::power_strip>;
} // namespace sokketter

#endif // LIBSOKKETTER_H
