#include "cli_parser.h"
#include "libsokketter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>
#include <vector>

using namespace testing;

namespace {
    auto set_test_device_number(const char *value) -> void
    {
#ifdef _WIN32
        _putenv_s("LIBSOKKETTER_TEST_DEVICE_NUMBER", value);
#else
        setenv("LIBSOKKETTER_TEST_DEVICE_NUMBER", value, 1);
#endif
    }

    auto unset_test_device_number() -> void
    {
#ifdef _WIN32
        _putenv_s("LIBSOKKETTER_TEST_DEVICE_NUMBER", "");
#else
        unsetenv("LIBSOKKETTER_TEST_DEVICE_NUMBER");
#endif
    }

    /**
     * @brief mirrors the CLI's client-side visibility rule: devices whose type bits intersect the
     * filter are visible, and TEST_DEVICE is always visible because the library returns fake
     * devices regardless of the filter.
     */
    bool is_visible_device(const std::shared_ptr<sokketter::power_strip> &device,
        const sokketter::power_strip_type &included_types)
    {
        using underlying_type = std::underlying_type_t<sokketter::power_strip_type>;

        const auto device_type = static_cast<underlying_type>(device->configuration().type);
        const auto test_device_type =
            static_cast<underlying_type>(sokketter::power_strip_type::TEST_DEVICE);

        return device_type == test_device_type
            || (device_type & static_cast<underlying_type>(included_types)) != 0;
    }

    std::shared_ptr<sokketter::power_strip> first_available_device()
    {
        sokketter::device_filter filter;
        filter.included_types = sokketter::power_strip_type::USB_DEVICES;

        for (const auto &device : sokketter::devices(filter))
        {
            if (is_visible_device(device, filter.included_types))
            {
                return device;
            }
        }

        return nullptr;
    }

    bool any_device_available(const sokketter::power_strip_type &included_types)
    {
        sokketter::device_filter filter;
        filter.included_types = included_types;

        return !sokketter::devices(filter).empty();
    }

    std::string expected_list_output()
    {
        sokketter::device_filter filter;
        filter.included_types = sokketter::power_strip_type::USB_DEVICES;

        std::vector<std::shared_ptr<sokketter::power_strip>> visible;
        for (const auto &device : sokketter::devices(filter))
        {
            if (is_visible_device(device, filter.included_types))
            {
                visible.push_back(device);
            }
        }

        if (visible.empty())
        {
            return "No devices found.\n";
        }

        std::ostringstream output;
        output << "Listing available devices...\n";
        output << "Available devices (indices start from 1):\n";

        for (size_t index = 0; index < visible.size(); ++index)
        {
            output << index + 1 << ". " << visible[index]->to_string() << "\n";
        }

        return output.str();
    }

    std::string expected_device_header(const std::shared_ptr<sokketter::power_strip> &device)
    {
        if (device == nullptr)
        {
            return "";
        }

        return device->to_string() + "\n";
    }

    std::string expected_socket_status_output(const std::shared_ptr<sokketter::power_strip> &device)
    {
        std::ostringstream output;
        if (device == nullptr)
        {
            return output.str();
        }

        size_t socket_index = 1;
        for (const auto &socket : device->sockets())
        {
            output << "  Socket " << socket_index << ": " << socket.to_string() << "\n";
            ++socket_index;
        }

        return output.str();
    }

    std::string expected_socket_action_output(
        const std::shared_ptr<sokketter::power_strip> &device, const std::string &action)
    {
        std::ostringstream output;
        if (device == nullptr)
        {
            return output.str();
        }

        size_t socket_index = 1;
        for (const auto &socket : device->sockets())
        {
            (void)socket;
            output << "  Socket " << socket_index << ": " << action << "\n";
            ++socket_index;
        }

        return output.str();
    }

    std::string expected_selected_socket_status_output(
        const std::shared_ptr<sokketter::power_strip> &device, const std::vector<size_t> &indices)
    {
        std::ostringstream output;
        if (device == nullptr)
        {
            return output.str();
        }

        for (const auto index : indices)
        {
            const auto &socket = device->sockets().at(index - 1);
            output << "  Socket " << index << ": " << socket.to_string() << "\n";
        }

        return output.str();
    }

    std::string expected_selected_socket_action_output(
        const std::shared_ptr<sokketter::power_strip> &device, const std::vector<size_t> &indices,
        const std::string &action)
    {
        std::ostringstream output;
        if (device == nullptr)
        {
            return output.str();
        }

        for (const auto index : indices)
        {
            (void)device;
            output << "  Socket " << index << ": " << action << "\n";
        }

        return output.str();
    }
} // namespace

TEST(cli_subcommand_tests, list_and_power_together)
{
    // MAN-CLI-06
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list", (char *)"power"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "The following argument was not expected: power\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, power_and_list_together)
{
    // MAN-CLI-06
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "A subcommand is required\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, list_no_devices)
{
    // MAN-CLI-04
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list"};

    set_test_device_number("0");

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, "Listing available devices...\n");
    ASSERT_EQ(err, "No devices found.\n");
}

TEST(cli_subcommand_tests, list_test_devices)
{
    // MAN-CLI-05
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list"};

    set_test_device_number("3");

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());
    const auto &expected_output = expected_list_output();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_output);
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, list_random_subcommand)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list", (char *)"random"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "The following argument was not expected: random\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, list_include_device_types_is_case_insensitive)
{
    std::vector<char *> args_lower = {
        (char *)"sokketter-cli", (char *)"list", (char *)"--include-device-types", (char *)"usb"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto return_code_lower =
        cli_parser::parse_and_process(args_lower.size(), args_lower.data());
    const auto out_lower = testing::internal::GetCapturedStdout();
    const auto err_lower = testing::internal::GetCapturedStderr();

    std::vector<char *> args_upper = {
        (char *)"sokketter-cli", (char *)"list", (char *)"--include-device-types", (char *)"USB"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto return_code_upper =
        cli_parser::parse_and_process(args_upper.size(), args_upper.data());
    const auto out_upper = testing::internal::GetCapturedStdout();
    const auto err_upper = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code_lower, return_code_upper);
    EXPECT_EQ(out_lower, out_upper);
    EXPECT_EQ(err_lower, err_upper);
}

TEST(cli_subcommand_tests, list_include_ethernet_device_types_is_case_insensitive)
{
    // MAN-CLI-19
    std::vector<char *> args_lower = {(char *)"sokketter-cli", (char *)"list",
        (char *)"--include-device-types", (char *)"ethernet"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto return_code_lower =
        cli_parser::parse_and_process(args_lower.size(), args_lower.data());
    const auto out_lower = testing::internal::GetCapturedStdout();
    const auto err_lower = testing::internal::GetCapturedStderr();

    std::vector<char *> args_upper = {(char *)"sokketter-cli", (char *)"list",
        (char *)"--include-device-types", (char *)"ETHERNET"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto return_code_upper =
        cli_parser::parse_and_process(args_upper.size(), args_upper.data());
    const auto out_upper = testing::internal::GetCapturedStdout();
    const auto err_upper = testing::internal::GetCapturedStderr();

    if (!any_device_available(sokketter::power_strip_type::ALL_DEVICES))
    {
        GTEST_SKIP() << "no devices are available";
    }

    ASSERT_EQ(return_code_lower, EXIT_SUCCESS);
    ASSERT_EQ(return_code_upper, EXIT_SUCCESS);
    EXPECT_EQ(return_code_lower, return_code_upper);
    EXPECT_EQ(out_lower, out_upper);
    EXPECT_EQ(err_lower, err_upper);
}

TEST(cli_subcommand_tests, list_include_lan_device_types_is_case_insensitive)
{
    // MAN-CLI-20
    std::vector<char *> args_lower = {
        (char *)"sokketter-cli", (char *)"list", (char *)"--include-device-types", (char *)"lan"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto return_code_lower =
        cli_parser::parse_and_process(args_lower.size(), args_lower.data());
    const auto out_lower = testing::internal::GetCapturedStdout();
    const auto err_lower = testing::internal::GetCapturedStderr();

    std::vector<char *> args_upper = {
        (char *)"sokketter-cli", (char *)"list", (char *)"--include-device-types", (char *)"LAN"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto return_code_upper =
        cli_parser::parse_and_process(args_upper.size(), args_upper.data());
    const auto out_upper = testing::internal::GetCapturedStdout();
    const auto err_upper = testing::internal::GetCapturedStderr();

    if (!any_device_available(sokketter::power_strip_type::ALL_DEVICES))
    {
        GTEST_SKIP() << "no devices are available";
    }

    ASSERT_EQ(return_code_lower, EXIT_SUCCESS);
    ASSERT_EQ(return_code_upper, EXIT_SUCCESS);
    EXPECT_EQ(return_code_lower, return_code_upper);
    EXPECT_EQ(out_lower, out_upper);
    EXPECT_EQ(err_lower, err_upper);
}

TEST(cli_subcommand_tests, list_include_device_types_rejects_unknown_type)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list",
        (char *)"--include-device-types", (char *)"ethernett"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto out = testing::internal::GetCapturedStdout();
    const auto err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "Listing available devices...\n");
    ASSERT_EQ(err, "Unknown device type: ethernett. Available types are: USB, ETHERNET, LAN.\n");
}

TEST(cli_subcommand_tests, list_include_device_types_rejects_substring_match)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"list", (char *)"--include-device-types", (char *)"island"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto out = testing::internal::GetCapturedStdout();
    const auto err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "Listing available devices...\n");
    ASSERT_EQ(err, "Unknown device type: island. Available types are: USB, ETHERNET, LAN.\n");
}

TEST(cli_subcommand_tests, list_include_device_types_accepts_comma_separated_tokens)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list",
        (char *)"--include-device-types", (char *)"USB, Ethernet , LAN"};

    set_test_device_number("0");

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto out = testing::internal::GetCapturedStdout();
    const auto err = testing::internal::GetCapturedStderr();

    /**
     * @attention all tokens are valid, so parsing must succeed and reach the listing stage.
     * With zero fake devices the run ends with "No devices found." rather than a validation error.
     */
    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, "Listing available devices...\n");
    ASSERT_EQ(err, "No devices found.\n");
}

TEST(cli_subcommand_tests, list_include_device_types_rejects_empty_value)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"list", (char *)"--include-device-types", (char *)""};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto out = testing::internal::GetCapturedStdout();
    const auto err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "Listing available devices...\n");
    ASSERT_EQ(err, "No device types were specified. Available types are: USB, ETHERNET, LAN.\n");
}

TEST(cli_subcommand_tests, list_include_device_types_rejects_unknown_token_in_list)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list",
        (char *)"--include-device-types", (char *)"usb,poland"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto out = testing::internal::GetCapturedStdout();
    const auto err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "Listing available devices...\n");
    ASSERT_EQ(err, "Unknown device type: poland. Available types are: USB, ETHERNET, LAN.\n");
}

TEST(cli_subcommand_tests, test_power_both_access_flags)
{
    // MAN-CLI-09
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-at-index", (char *)"1", (char *)"--device-with-serial", (char *)"TEST"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "--device-at-index excludes --device-with-serial\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, test_power_no_access_flags)
{
    // MAN-CLI-10
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "[Option Group: --device-at-index or --device-with-serial] is required.\nRun "
                   "with --help for more information.\n");
}

TEST(cli_subcommand_tests, test_power_status_no_device)
{
    // MAN-CLI-11
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-with-serial", (char *)"TEST2"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "No device was found for serial \"TEST2\".\n");
}

TEST(cli_subcommand_tests, test_power_status_via_index)
{
    // MAN-CLI-07
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-at-index", (char *)"1", (char *)"--sockets", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto device = first_available_device();
    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, test_power_zero_device_index)
{
    // MAN-CLI-16
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "Device index 0 is out of range (valid range starts from 1).\n");
}

TEST(cli_subcommand_tests, test_power_status_via_serial)
{
    // MAN-CLI-08
    const auto device = first_available_device();
    std::string serial = device != nullptr ? device->configuration().id : "missing";

    std::vector<std::string> arguments = {
        "sokketter-cli", "power", "status", "--device-with-serial", serial, "--sockets", "1"};
    std::vector<char *> args;
    for (auto &argument : arguments)
    {
        args.push_back(argument.data());
    }

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code =
        cli_parser::parse_and_process(static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, test_power_on_specified_socket)
{
    // MAN-CLI-13
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"on",
        (char *)"--device-at-index", (char *)"1", (char *)"--sockets", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto device = first_available_device();
    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_device_header(device) +
                       expected_selected_socket_action_output(device, {1}, "turned on."));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, test_power_off_specified_socket)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"off",
        (char *)"--device-at-index", (char *)"1", (char *)"--sockets", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto device = first_available_device();
    if (device == nullptr)
    {
        ASSERT_EQ(return_code, EXIT_FAILURE);
        ASSERT_EQ(out, "");
        ASSERT_EQ(err, "Device index 1 is out of range (no devices are available).\n");
    }
    else
    {
        ASSERT_EQ(return_code, EXIT_SUCCESS);
        ASSERT_EQ(out, expected_device_header(device) +
                           expected_selected_socket_action_output(device, {1}, "turned off."));
        ASSERT_EQ(err, "");
    }
}

TEST(cli_subcommand_tests, test_power_toggle_specified_socket)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"1", (char *)"--sockets", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto device = first_available_device();
    if (device == nullptr)
    {
        ASSERT_EQ(return_code, EXIT_FAILURE);
        ASSERT_EQ(out, "");
        ASSERT_EQ(err, "Device index 1 is out of range (no devices are available).\n");
    }
    else
    {
        ASSERT_EQ(return_code, EXIT_SUCCESS);
        ASSERT_EQ(out, expected_device_header(device) +
                           expected_selected_socket_action_output(device, {1}, "toggled."));
        ASSERT_EQ(err, "");
    }
}

TEST(cli_subcommand_tests, test_power_status_all)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-at-index", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto device = first_available_device();
    if (device == nullptr)
    {
        ASSERT_EQ(return_code, EXIT_FAILURE);
        ASSERT_EQ(out, "");
        ASSERT_EQ(err, "Device index 1 is out of range (no devices are available).\n");
    }
    else
    {
        ASSERT_EQ(return_code, EXIT_SUCCESS);
        ASSERT_EQ(out, expected_device_header(device) + expected_socket_status_output(device));
        ASSERT_EQ(err, "");
    }
}

TEST(cli_subcommand_tests, test_power_on_all)
{
    // MAN-CLI-12
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"on",
        (char *)"--device-at-index", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto device = first_available_device();
    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_socket_action_output(device, "turned on."));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, test_power_off_all)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"off",
        (char *)"--device-at-index", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto device = first_available_device();
    if (device == nullptr)
    {
        ASSERT_EQ(return_code, EXIT_FAILURE);
        ASSERT_EQ(out, "");
        ASSERT_EQ(err, "Device index 1 is out of range (no devices are available).\n");
    }
    else
    {
        ASSERT_EQ(return_code, EXIT_SUCCESS);
        ASSERT_EQ(out,
            expected_device_header(device) + expected_socket_action_output(device, "turned off."));
        ASSERT_EQ(err, "");
    }
}

TEST(cli_subcommand_tests, test_power_toggle_all)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto device = first_available_device();
    if (device == nullptr)
    {
        ASSERT_EQ(return_code, EXIT_FAILURE);
        ASSERT_EQ(out, "");
        ASSERT_EQ(err, "Device index 1 is out of range (no devices are available).\n");
    }
    else
    {
        ASSERT_EQ(return_code, EXIT_SUCCESS);
        ASSERT_EQ(out,
            expected_device_header(device) + expected_socket_action_output(device, "toggled."));
        ASSERT_EQ(err, "");
    }
}

TEST(cli_subcommand_tests, test_power_too_big_socket_index)
{
    // MAN-CLI-16
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"1", (char *)"--sockets", (char *)"99"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto device = first_available_device();
    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "Socket index 99 is out of range (valid range: 1-" +
            std::to_string(device->sockets().size()) + ").\n");
}

TEST(cli_subcommand_tests, test_power_zero_socket_index)
{
    // MAN-CLI-16
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"1", (char *)"--sockets", (char *)"0"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto device = first_available_device();
    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "Socket index 0 is out of range (valid range: 1-" +
            std::to_string(device->sockets().size()) + ").\n");
}

TEST(cli_subcommand_tests, test_power_negative_socket_index)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"1", (char *)"--sockets", (char *)"-1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "Could not convert: --sockets = -1\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, test_power_random_subcommand)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"random"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "A subcommand is required\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, test_power_on_random_subcommand)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"on", (char *)"random"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "The following argument was not expected: random\nRun with --help for more "
                   "information.\n");
}

TEST(cli_subcommand_tests, test_power_off_multiple_sockets)
{
    // MAN-CLI-14
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"off",
        (char *)"--device-at-index", (char *)"1", (char *)"--sockets", (char *)"1", (char *)"2"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto device = first_available_device();
    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_device_header(device) +
                       expected_selected_socket_action_output(device, {1, 2}, "turned off."));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, test_power_toggle_double_restores_state)
{
    // MAN-CLI-15
    const auto device = first_available_device();
    const std::string initial_status =
        device != nullptr ? expected_selected_socket_status_output(device, {1}) : "";

    std::vector<char *> toggle_args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"1", (char *)"--sockets", (char *)"1"};

    for (int i = 0; i < 2; ++i)
    {
        testing::internal::CaptureStdout();
        testing::internal::CaptureStderr();
        const auto &rc = cli_parser::parse_and_process(toggle_args.size(), toggle_args.data());
        testing::internal::GetCapturedStdout();
        testing::internal::GetCapturedStderr();
        if (device == nullptr)
        {
            ASSERT_EQ(rc, EXIT_FAILURE);
            return;
        }
        ASSERT_EQ(rc, EXIT_SUCCESS);
    }

    std::vector<char *> status_args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-at-index", (char *)"1", (char *)"--sockets", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    const auto &rc = cli_parser::parse_and_process(status_args.size(), status_args.data());
    const auto &out = testing::internal::GetCapturedStdout();
    testing::internal::GetCapturedStderr();

    ASSERT_EQ(rc, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_device_header(device) + initial_status);
}

TEST(cli_subcommand_tests, mixed_case_list_subcommand)
{
    // MAN-CLI-18
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"LIST"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto expected_output = expected_list_output();
    if (expected_output == "No devices found.\n")
    {
        GTEST_SKIP() << "no devices are available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_output);
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, underscore_option_syntax)
{
    // MAN-CLI-18
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device_at_index", (char *)"1", (char *)"--sockets", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto device = first_available_device();
    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, power_status_short_flags)
{
    // MAN-CLI-07 via short spellings -i / -s
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"1", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto device = first_available_device();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, power_status_short_serial_flag)
{
    // MAN-CLI-08 via short spelling -n
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-n", (char *)"TEST_SERIAL_NUMBER_0", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto device = first_available_device();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, power_status_underscored_serial_option_keeps_value)
{
    // MAN-CLI-18: --device_with_serial is aliased, but its underscored value must not be mutated.
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device_with_serial", (char *)"TEST_SERIAL_NUMBER_0", (char *)"--sockets",
        (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto device = first_available_device();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, power_status_windows_style_options)
{
    // MAN-CLI-18: windows-style spellings of the power options.
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"/device-at-index", (char *)"1", (char *)"/sockets", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto device = first_available_device();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, power_status_windows_style_underscored_options)
{
    // MAN-CLI-18: underscored Windows-style spellings are aliased to the dashed option names.
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"/device_at_index", (char *)"1", (char *)"/sockets", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto device = first_available_device();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, power_status_windows_style_underscored_serial_option_keeps_value)
{
    // MAN-CLI-18: /device_with_serial is aliased, but its underscored value must not be mutated.
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"/device_with_serial", (char *)"TEST_SERIAL_NUMBER_0", (char *)"/sockets",
        (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto device = first_available_device();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, mixed_case_power_subcommands)
{
    // MAN-CLI-18: mixed-case spellings of power and its action subcommands.
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"POWER", (char *)"Status",
        (char *)"-i", (char *)"1", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto device = first_available_device();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, power_on_via_serial)
{
    // MAN-CLI-13 selected by serial instead of index.
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"on",
        (char *)"-n", (char *)"TEST_SERIAL_NUMBER_0", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto device = first_available_device();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_device_header(device) +
                       expected_selected_socket_action_output(device, {1}, "turned on."));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, power_off_via_serial)
{
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"off",
        (char *)"-n", (char *)"TEST_SERIAL_NUMBER_0", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto device = first_available_device();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_device_header(device) +
                       expected_selected_socket_action_output(device, {1}, "turned off."));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, power_toggle_via_serial)
{
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"-n", (char *)"TEST_SERIAL_NUMBER_0", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto device = first_available_device();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_device_header(device) +
                       expected_selected_socket_action_output(device, {1}, "toggled."));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, power_toggle_multiple_sockets)
{
    // MAN-CLI-14 for the toggle action.
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"-i", (char *)"1", (char *)"-s", (char *)"1", (char *)"2"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto device = first_available_device();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_device_header(device) +
                       expected_selected_socket_action_output(device, {1, 2}, "toggled."));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, power_duplicate_socket_indices_applied_once)
{
    // --sockets 1 1 must apply the action to socket 1 exactly once.
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"on",
        (char *)"-i", (char *)"1", (char *)"-s", (char *)"1", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto device = first_available_device();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_device_header(device) +
                       expected_selected_socket_action_output(device, {1}, "turned on."));
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, power_validates_all_socket_indices_before_action)
{
    // An invalid index in the list must abort the whole command before any socket is touched.
    set_test_device_number("1");

    const auto device = first_available_device();
    if (device == nullptr)
    {
        unset_test_device_number();
        GTEST_SKIP() << "no device is available";
    }

    const std::string initial_status = expected_selected_socket_status_output(device, {1});

    std::vector<char *> power_args = {(char *)"sokketter-cli", (char *)"power", (char *)"on",
        (char *)"-i", (char *)"1", (char *)"-s", (char *)"1", (char *)"99"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &power_code = cli_parser::parse_and_process(power_args.size(), power_args.data());
    const auto &power_out = testing::internal::GetCapturedStdout();
    const auto &power_err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(power_code, EXIT_FAILURE);
    ASSERT_EQ(power_out, "");
    ASSERT_EQ(power_err,
        "Socket index 99 is out of range (valid range: 1-" +
            std::to_string(device->sockets().size()) + ").\n");

    std::vector<char *> status_args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"1", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &status_code = cli_parser::parse_and_process(status_args.size(), status_args.data());
    const auto &status_out = testing::internal::GetCapturedStdout();
    testing::internal::GetCapturedStderr();

    unset_test_device_number();

    ASSERT_EQ(status_code, EXIT_SUCCESS);
    ASSERT_EQ(status_out, expected_device_header(device) + initial_status);
}

TEST(cli_subcommand_tests, power_device_index_out_of_range)
{
    // A valid-format index beyond the visible device count reports the positional range.
    set_test_device_number("2");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"3", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "Device index 3 is out of range (valid range: 1-2).\n");
}

TEST(cli_subcommand_tests, power_device_index_conversion_error)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"abc", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "Could not convert: --device-at-index = abc\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, power_sockets_missing_value_error)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"status", (char *)"-i", (char *)"1",
        (char *)"-s"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_THAT(err, StartsWith("--sockets"));
    ASSERT_THAT(err, HasSubstr("Run with --help for more information."));
}

TEST(cli_subcommand_tests, list_include_device_types_short_flag)
{
    // MAN-CLI-19 via short spelling -t
    set_test_device_number("2");

    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"list", (char *)"-t", (char *)"usb"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());
    const auto &expected_output = expected_list_output();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_output);
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, list_include_device_types_underscore_spelling)
{
    // MAN-CLI-18: --include_device_types must alias to --include-device-types.
    set_test_device_number("2");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list",
        (char *)"--include_device_types", (char *)"usb"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());
    const auto &expected_output = expected_list_output();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_output);
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, list_include_ethernet_device_types_with_fake_devices)
{
    // MAN-CLI-19: the ethernet filter is accepted and reaches the listing stage.
    set_test_device_number("2");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list",
        (char *)"--include-device-types", (char *)"ethernet"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());
    const auto &expected_output = expected_list_output();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_output);
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, list_include_lan_device_types_with_fake_devices)
{
    // MAN-CLI-20: the lan alias behaves the same as ethernet.
    set_test_device_number("2");

    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"list", (char *)"--include-device-types", (char *)"lan"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());
    const auto &expected_output = expected_list_output();

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_output);
    ASSERT_EQ(err, "");
}

TEST(cli_subcommand_tests, list_include_device_types_skips_empty_tokens)
{
    /**
     * @attention review finding 6: empty tokens between commas must be skipped rather than
     * reported as an unknown type. Expected behavior per the review document; the remaining
     * tokens are valid, so the run must reach the listing stage.
     */
    set_test_device_number("0");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list",
        (char *)"--include-device-types", (char *)"usb,,ethernet"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, "Listing available devices...\n");
    ASSERT_EQ(err, "No devices found.\n");
}

TEST(cli_subcommand_tests, power_without_action_subcommand_is_rejected)
{
    /**
     * @attention review finding 1: `power` alone must not be a silent no-op with exit 0.
     * Expected behavior per the review document: power requires exactly one action subcommand.
     */
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"-i", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "A subcommand is required\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, power_with_two_action_subcommands_is_rejected)
{
    /**
     * @attention review finding 1: contradictory actions must not both run. Expected behavior
     * per the review document: at most one action subcommand is accepted.
     */
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"on",
        (char *)"off", (char *)"-i", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "The following argument was not expected: off\nRun with --help for more information.\n");
}
