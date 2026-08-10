#include "cli_parser.h"
#include "libsokketter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>
#include <vector>

using namespace testing;

namespace {
    std::shared_ptr<sokketter::power_strip> first_available_device()
    {
        const auto &devices = sokketter::devices();
        if (devices.empty())
        {
            return nullptr;
        }

        return devices.front();
    }

    std::string expected_list_output()
    {
        const auto &devices = sokketter::devices();
        if (devices.empty())
        {
            return "No devices found.\n";
        }

        std::ostringstream output;
        output << "Available devices:\n";

        for (size_t index = 0; index < devices.size(); ++index)
        {
            output << index + 1 << ". " << devices[index]->to_string() << "\n";
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
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list", (char *)"power"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, 109);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "The following argument was not expected: power\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, power_and_list_together)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, 109);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "The following argument was not expected: list\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, list_no_devices)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    const auto expected_output = expected_list_output();
    if (expected_output == "No devices found.\n")
    {
        ASSERT_EQ(return_code, EXIT_FAILURE);
        ASSERT_EQ(out, "");
        ASSERT_EQ(err, "No devices found.\n");
    }
    else
    {
        ASSERT_EQ(return_code, EXIT_SUCCESS);
        ASSERT_EQ(out, expected_output);
        ASSERT_EQ(err, "");
    }
}

TEST(cli_subcommand_tests, list_test_devices)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_list_output());
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

    ASSERT_EQ(return_code, 109);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "The following argument was not expected: random\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, test_power_both_access_flags)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-at-index", (char *)"0", (char *)"--device-with-serial", (char *)"TEST"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, 108);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "--device-at-index excludes --device-with-serial\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, test_power_no_access_flags)
{
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
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-with-serial", (char *)"TEST2"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "No device was found.\n");
}

TEST(cli_subcommand_tests, test_power_status_via_index)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"1"};

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
        ASSERT_EQ(err, "No device was found.\n");
    }
    else
    {
        ASSERT_EQ(return_code, EXIT_SUCCESS);
        ASSERT_EQ(out,
            expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
        ASSERT_EQ(err, "");
    }
}

TEST(cli_subcommand_tests, test_power_status_via_serial)
{
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
        ASSERT_EQ(return_code, EXIT_FAILURE);
        ASSERT_EQ(out, "");
        ASSERT_EQ(err, "No device was found.\n");
    }
    else
    {
        ASSERT_EQ(return_code, EXIT_SUCCESS);
        ASSERT_EQ(out,
            expected_device_header(device) + expected_selected_socket_status_output(device, {1}));
        ASSERT_EQ(err, "");
    }
}

TEST(cli_subcommand_tests, test_power_on_specified_socket)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"on",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"1"};

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
        ASSERT_EQ(err, "No device was found.\n");
    }
    else
    {
        ASSERT_EQ(return_code, EXIT_SUCCESS);
        ASSERT_EQ(out, expected_device_header(device) +
                           expected_selected_socket_action_output(device, {1}, "turned on."));
        ASSERT_EQ(err, "");
    }
}

TEST(cli_subcommand_tests, test_power_off_specified_socket)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"off",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"1"};

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
        ASSERT_EQ(err, "No device was found.\n");
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
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"1"};

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
        ASSERT_EQ(err, "No device was found.\n");
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
        (char *)"--device-at-index", (char *)"0"};

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
        ASSERT_EQ(err, "No device was found.\n");
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
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"on",
        (char *)"--device-at-index", (char *)"0"};

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
        ASSERT_EQ(err, "No device was found.\n");
    }
    else
    {
        ASSERT_EQ(return_code, EXIT_SUCCESS);
        ASSERT_EQ(out,
            expected_device_header(device) + expected_socket_action_output(device, "turned on."));
        ASSERT_EQ(err, "");
    }
}

TEST(cli_subcommand_tests, test_power_off_all)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"off",
        (char *)"--device-at-index", (char *)"0"};

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
        ASSERT_EQ(err, "No device was found.\n");
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
        (char *)"--device-at-index", (char *)"0"};

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
        ASSERT_EQ(err, "No device was found.\n");
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
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"99"};

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
        ASSERT_EQ(err, "No device was found.\n");
    }
    else
    {
        ASSERT_EQ(return_code, EXIT_FAILURE);
        ASSERT_EQ(out, expected_device_header(device));
        ASSERT_EQ(err, "Socket index 99 is out of range.\n");
    }
}

TEST(cli_subcommand_tests, test_power_zero_socket_index)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"0"};

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
        ASSERT_EQ(err, "No device was found.\n");
    }
    else
    {
        ASSERT_EQ(return_code, EXIT_FAILURE);
        ASSERT_EQ(out, expected_device_header(device));
        ASSERT_EQ(err, "Socket index 0 is out of range.\n");
    }
}

TEST(cli_subcommand_tests, test_power_negative_socket_index)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"-1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, 104);
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

    ASSERT_EQ(return_code, 109);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "The following argument was not expected: random\nRun with --help for more "
                   "information.\n");
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

    ASSERT_EQ(return_code, 109);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "The following argument was not expected: random\nRun with --help for more "
                   "information.\n");
}
