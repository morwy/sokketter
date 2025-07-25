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
     * @brief initializes sokketter library.
     * @return true in case of success, false in case of any failure.
     * @attention This function should be called before any other functions of the library.
     */
    auto EXPORTED initialize() -> bool;

    /**
     * @brief deinitializes sokketter library.
     * @return true in case of success, false in case of any failure.
     * @attention This function should be called after all other functions of the library.
     */
    auto EXPORTED deinitialize() -> bool;

    /**
     * @brief path to the folder where all related files are stored.
     * @return std::filesystem::path.
     */
    auto EXPORTED storage_path() -> std::filesystem::path;

    /**
     * @brief path to the folder where all logs are stored.
     * @return std::filesystem::path.
     */
    auto EXPORTED logs_path() -> std::filesystem::path;

    /**
     * @brief states levels of logging.
     */
    enum class logging_level : uint8_t
    {
        TRACE = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERR = 4,
        CRIT = 5,
        OFF = 6
    };

    /**
     * @brief
     */
    struct EXPORTED callback_response_structure
    {
        sokketter::logging_level level = sokketter::logging_level::ERR;
        const char *file = nullptr;
        int line = 0;
        const char *function = nullptr;
        std::string message = "";
    };

    /**
     * @brief
     */
    struct EXPORTED settings_structure
    {
        sokketter::logging_level logging_level = sokketter::logging_level::ERR;
        std::function<void(callback_response_structure)> logging_callback = nullptr;
        std::string logging_pattern = "%+";
    };

    /**
     * @brief gets current settings of kommpot library.
     * @return settings structure.
     */
    auto EXPORTED settings() noexcept -> settings_structure;

    /**
     * @brief sets the new settings of kommpot library.
     * @param settings structure.
     */
    auto EXPORTED set_settings(const settings_structure &settings) noexcept -> void;

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
     * @brief structure containing configuration parameters of the specific socket.
     */
    struct EXPORTED socket_configuration
    {
        std::string name = "Unnamed socket";
        std::string description = "";

        /**
         * @brief configurable reset timeout in milliseconds.
         * @attention 0 means disabled.
         */
        uint32_t configurable_reset_msec = 0;
    };

    /**
     * @brief the class for controlling and configuring the socket.
     */
    class EXPORTED socket
    {
    public:
        socket(const socket_configuration &configuration);
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

        power_strip(const power_strip &obj) = default;
        auto operator=(const power_strip &obj) -> power_strip & = default;

        power_strip(power_strip &&obj) = default;
        auto operator=(power_strip &&obj) -> power_strip & = default;

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
         * @brief saves current configuration of the power strip to the storage.
         */
        auto save() -> void;

        /**
         * @brief gets connection state of the power strip.
         * @return bool if device is connected, false if device is not connected.
         */
        [[nodiscard]] virtual auto is_connected() const -> bool;

        /**
         * @brief gets list of sockets controlled by the power strip.
         * @return vector of socket objects.
         */
        [[nodiscard]] virtual auto sockets() -> std::vector<socket> &;

        /**
         * @brief gets list of sockets controlled by the power strip.
         * @return vector of socket objects.
         */
        [[nodiscard]] virtual auto sockets() const -> const std::vector<socket> &;

        /**
         * @brief gets a specific socket controlled by the power strip.
         * @param index of the socket.
         * @return socket object as std::optional in case of success, std::nullopt in case of
         * failure.
         */
        [[nodiscard]] virtual auto socket(const size_t &index)
            -> const std::optional<std::reference_wrapper<socket>>;

        /**
         * @brief creates string based on power strip parameters
         * @return string in format "POWER_STRIP_NAME (TYPE, ID, located at ADDRESS)".
         */
        [[nodiscard]] auto to_string() const noexcept -> std::string;

    protected:
        std::vector<sokketter::socket> m_sockets;

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
        -> const std::vector<std::shared_ptr<sokketter::power_strip>> &;

    /**
     * @brief returns the power strip device by its index.
     * @param index of the device.
     * @return unique pointer to power_strip object or nullptr in case of any failure.
     */
    auto EXPORTED device(const size_t &index) -> std::shared_ptr<sokketter::power_strip>;

    /**
     * @brief returns the power strip device by its serial number.
     * @param serial_number of the device.
     * @return unique pointer to power_strip object or nullptr in case of any failure.
     */
    auto EXPORTED device(const std::string &serial_number)
        -> std::shared_ptr<sokketter::power_strip>;
} // namespace sokketter

#endif // LIBSOKKETTER_H
