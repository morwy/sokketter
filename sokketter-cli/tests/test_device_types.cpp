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
     * @brief mirrors the CLI's visible_devices(): enumerate through the library with the given
     * filter, then apply the client-side visibility rule (TEST_DEVICE stays visible regardless).
     */
    std::vector<std::shared_ptr<sokketter::power_strip>> visible_devices_like_cli(
        const sokketter::power_strip_type &included_types)
    {
        using underlying_type = std::underlying_type_t<sokketter::power_strip_type>;

        sokketter::device_filter filter;
        filter.included_types = included_types;

        std::vector<std::shared_ptr<sokketter::power_strip>> visible;
        for (const auto &device : sokketter::devices(filter))
        {
            const auto device_type = static_cast<underlying_type>(device->configuration().type);
            const auto test_device_type =
                static_cast<underlying_type>(sokketter::power_strip_type::TEST_DEVICE);

            if (device_type == test_device_type
                || (device_type & static_cast<underlying_type>(included_types)) != 0)
            {
                visible.push_back(device);
            }
        }

        return visible;
    }

    /**
     * @brief returns the 1-based visible device the CLI would resolve --device-at-index to, or
     * nullptr when the index is out of range.
     */
    std::shared_ptr<sokketter::power_strip> cli_device_at_index(const size_t index)
    {
        const auto visible = visible_devices_like_cli(sokketter::power_strip_type::USB_DEVICES);

        if (index == 0 || index > visible.size())
        {
            return nullptr;
        }

        return visible[index - 1];
    }

    std::string expected_device_header(const std::shared_ptr<sokketter::power_strip> &device)
    {
        if (device == nullptr)
        {
            return "";
        }

        return device->to_string() + "\n";
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
        const std::vector<size_t> &indices, const std::string &action)
    {
        std::ostringstream output;

        for (const auto index : indices)
        {
            output << "  Socket " << index << ": " << action << "\n";
        }

        return output.str();
    }
} // namespace

/**
 * @brief coverage for --include-device-types on the power subcommand (previously untested) plus
 * token edge inputs, boundary values and defensive parsing cases.
 */

TEST(cli_device_type_tests, power_include_device_types_accepts_usb)
{
    // MAN-CLI-22: the option is accepted on power and index resolution runs against the selection.
    set_test_device_number("5");

    const auto device = cli_device_at_index(5);
    if (device == nullptr)
    {
        unset_test_device_number();
        GTEST_SKIP() << "no fake device is available";
    }

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"5", (char *)"-t", (char *)"usb", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_device_type_tests, power_include_device_types_ethernet_keeps_test_devices_by_index)
{
    // MAN-CLI-23: TEST_DEVICE stays addressable by index even under an ethernet-only selection.
    set_test_device_number("5");

    const auto device = cli_device_at_index(5);
    if (device == nullptr)
    {
        unset_test_device_number();
        GTEST_SKIP() << "no fake device is available";
    }

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"5", (char *)"-t", (char *)"ethernet", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_device_type_tests, power_include_device_types_ethernet_keeps_test_devices_by_serial)
{
    // MAN-CLI-24: TEST_DEVICE stays addressable by serial even under an ethernet-only selection.
    set_test_device_number("5");

    const auto device = cli_device_at_index(5);
    if (device == nullptr)
    {
        unset_test_device_number();
        GTEST_SKIP() << "no fake device is available";
    }

    std::vector<std::string> arguments = {"sokketter-cli", "power", "status", "--device-with-serial",
        device->configuration().id, "--include-device-types", "ethernet", "--sockets", "1"};
    std::vector<char *> args;
    for (auto &argument : arguments)
    {
        args.push_back(argument.data());
    }

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code =
        cli_parser::parse_and_process(static_cast<int>(args.size()), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
    ASSERT_EQ(err, "");
}

TEST(cli_device_type_tests, power_include_device_types_rejects_unknown_type)
{
    // MAN-CLI-25: the power path reports the same token diagnostics as the list path.
    set_test_device_number("2");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"1", (char *)"-t", (char *)"garbage", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "Unknown device type: garbage. Available types are: USB, ETHERNET, LAN.\n");
}

TEST(cli_device_type_tests, power_index_no_devices_available)
{
    // MAN-CLI-29: the index path distinguishes "no devices" from "index beyond range".
    set_test_device_number("0");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"1", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "Device index 1 is out of range (no devices are available).\n");
}

TEST(cli_device_type_tests, power_device_index_degenerate_range)
{
    // B-03: with exactly one visible device the reported range is the degenerate "1-1".
    set_test_device_number("1");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"2", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "Device index 2 is out of range (valid range: 1-1).\n");
}

TEST(cli_device_type_tests, list_include_device_types_rejects_leading_comma)
{
    // MAN-CLI-26: a leading comma yields an empty token.
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list",
        (char *)"--include-device-types", (char *)",usb"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "Listing available devices...\n");
    ASSERT_EQ(err, "Empty device type in --include-device-types. Available types are: USB, "
                   "ETHERNET, LAN.\n");
}

TEST(cli_device_type_tests, list_include_device_types_rejects_whitespace_only_token)
{
    // MAN-CLI-27: a whitespace-only token is stripped to empty and rejected.
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list",
        (char *)"--include-device-types", (char *)" "};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "Listing available devices...\n");
    ASSERT_EQ(err, "Empty device type in --include-device-types. Available types are: USB, "
                   "ETHERNET, LAN.\n");
}

TEST(cli_device_type_tests, list_include_device_types_accepts_equals_form)
{
    // MAN-CLI-28: the key=value form is accepted; normalization stops at '='.
    set_test_device_number("2");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list",
        (char *)"--include-device-types=usb"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(out, HasSubstr("Available devices (indices start from 1):"));
    ASSERT_EQ(err, "");
}

TEST(cli_device_type_tests, list_include_device_types_windows_style_underscored)
{
    // P-07: /include_device_types is aliased to --include-device-types via windows options plus
    // the underscore-to-dash normalization pass.
    set_test_device_number("2");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list",
        (char *)"/include_device_types", (char *)"usb"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(out, HasSubstr("Available devices (indices start from 1):"));
    ASSERT_EQ(err, "");
}

TEST(cli_device_type_tests, power_socket_index_upper_boundary_valid)
{
    // B-01: the last socket index (fake devices have 4 sockets) is accepted.
    set_test_device_number("5");

    const auto device = cli_device_at_index(5);
    if (device == nullptr)
    {
        unset_test_device_number();
        GTEST_SKIP() << "no fake device is available";
    }

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"5", (char *)"-s", (char *)"4"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(
        out, expected_device_header(device) + expected_selected_socket_status_output(device, {4}));
    ASSERT_EQ(err, "");
}

TEST(cli_device_type_tests, power_socket_index_above_maximum)
{
    // B-02: one past the last socket index reports the per-device range.
    set_test_device_number("5");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"5", (char *)"-s", (char *)"5"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "Socket index 5 is out of range (valid range: 1-4).\n");
}

TEST(cli_device_type_tests, power_socket_index_size_max_parses)
{
    // B-06: SIZE_MAX is a valid size_t conversion, so it reaches the range check.
    set_test_device_number("5");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"5", (char *)"-s", (char *)"18446744073709551615"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "Socket index 18446744073709551615 is out of range (valid range: 1-4).\n");
}

TEST(cli_device_type_tests, power_device_index_size_max_parses)
{
    // B-07: SIZE_MAX device index converts and reaches the positional range check.
    set_test_device_number("2");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"18446744073709551615", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "Device index 18446744073709551615 is out of range (valid range: 1-2).\n");
}

TEST(cli_device_type_tests, power_device_index_overflow_conversion_error)
{
    // B-08: SIZE_MAX+1 overflows size_t and fails the CLI11 conversion instead.
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-i", (char *)"18446744073709551616", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "Could not convert: --device-at-index = 18446744073709551616\nRun with --help for more "
        "information.\n");
}

TEST(cli_device_type_tests, power_empty_serial_not_found)
{
    // N-30: an empty serial is a well-formed lookup that simply misses.
    set_test_device_number("2");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-n", (char *)"", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "No device was found for serial \"\".\n");
}

TEST(cli_device_type_tests, power_socket_indices_printed_in_sorted_order)
{
    // E-15: unsorted --sockets input is sorted before the action is applied and printed.
    set_test_device_number("6");

    const auto device = cli_device_at_index(6);
    if (device == nullptr)
    {
        unset_test_device_number();
        GTEST_SKIP() << "no fake device is available";
    }

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"on", (char *)"-i",
        (char *)"6", (char *)"-s", (char *)"2", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out,
        expected_device_header(device) + expected_selected_socket_action_output({1, 2}, "turned on."));
    ASSERT_EQ(err, "");
}

TEST(cli_device_type_tests, argc_zero_returns_failure)
{
    // E-10: the empty-argument-vector guard returns failure without touching argv.
    const auto &return_code = cli_parser::parse_and_process(0, nullptr);

    ASSERT_EQ(return_code, EXIT_FAILURE);
}

TEST(cli_device_type_tests, repeated_list_subcommand_is_rejected)
{
    // E-13: require_subcommand(1, 1) rejects a repeated subcommand token.
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_THAT(err, HasSubstr("The following argument was not expected: list"));
}

TEST(cli_device_type_tests, repeated_power_action_subcommand_is_rejected)
{
    // E-14: require_subcommand(1, 1) on power rejects a repeated action token.
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"status", (char *)"status"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_THAT(err, HasSubstr("The following argument was not expected: status"));
}

/**
 * @brief captures the list output for the current device configuration (env var left as-is).
 */
auto capture_list_output() -> std::string
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    EXPECT_EQ(return_code, EXIT_SUCCESS);
    EXPECT_EQ(err, "");

    return out;
}

TEST(cli_device_type_tests, invalid_test_device_number_env_behaves_like_unset)
{
    /**
     * @attention E-18: an unparsable LIBSOKKETTER_TEST_DEVICE_NUMBER falls back to the unset
     * sentinel, so the run must match the unset baseline exactly (which may be real hardware on a
     * developer machine or "No devices found." on a clean runner).
     */
    unset_test_device_number();
    const auto baseline = capture_list_output();

    set_test_device_number("abc");
    const auto invalid = capture_list_output();

    unset_test_device_number();

    ASSERT_EQ(invalid, baseline);
}

TEST(cli_device_type_tests, huge_test_device_number_env_behaves_like_unset)
{
    /**
     * @attention E-19: an out-of-range LIBSOKKETTER_TEST_DEVICE_NUMBER falls back to the unset
     * sentinel, so the run must match the unset baseline exactly.
     */
    unset_test_device_number();
    const auto baseline = capture_list_output();

    set_test_device_number("99999999999999999999999");
    const auto huge = capture_list_output();

    unset_test_device_number();

    ASSERT_EQ(huge, baseline);
}

TEST(cli_device_type_tests, slash_prefixed_option_value_is_normalized)
{
    /**
     * @attention E-03: pins the documented normalization gotcha: values that begin with '/' are
     * treated as windows-style option tokens and have underscores rewritten to dashes, so a serial
     * such as /dev/ttyUSB_0 is searched for as /dev/ttyUSB-0 and reported as not found.
     */
    set_test_device_number("2");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"-n", (char *)"/dev/ttyUSB_0", (char *)"-s", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    unset_test_device_number();

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "No device was found for serial \"/dev/ttyUSB-0\".\n");
}

TEST(cli_device_type_tests, serial_outside_device_type_selection_is_not_found)
{
    /**
     * @attention N-34: hardware-conditional. A real USB device's serial restricted by an
     * ethernet-only selection must be reported as not found, mirroring how the index path cannot
     * reach such devices. Skips when no real (non-fake) USB device is attached; graduates to the
     * hardware tier once a dedicated hardware target exists.
     */
    /**
     * @attention select a real USB device specifically: under an ethernet-only selection its
     * serial must be reported as not found. A real ethernet device would match the selection and
     * be powered for real, so it must not be used here.
     */
    using underlying_type = std::underlying_type_t<sokketter::power_strip_type>;
    const auto usb_mask = static_cast<underlying_type>(sokketter::power_strip_type::USB_DEVICES);

    std::shared_ptr<sokketter::power_strip> real_usb_device;
    for (const auto &device : visible_devices_like_cli(sokketter::power_strip_type::ALL_DEVICES))
    {
        const auto device_type = static_cast<underlying_type>(device->configuration().type);

        if (device_type != static_cast<underlying_type>(sokketter::power_strip_type::TEST_DEVICE)
            && (device_type & usb_mask) != 0)
        {
            real_usb_device = device;
            break;
        }
    }

    if (real_usb_device == nullptr)
    {
        GTEST_SKIP() << "no real USB device is attached";
    }

    std::vector<std::string> arguments = {"sokketter-cli", "power", "status", "--device-with-serial",
        real_usb_device->configuration().id, "--include-device-types", "ethernet", "--sockets", "1"};
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

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "No device was found for serial \"" + real_usb_device->configuration().id + "\".\n");
}
