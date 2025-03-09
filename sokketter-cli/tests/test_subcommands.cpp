#include "cli_parser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

#ifdef _WIN32
#    include <windows.h>
#endif

using namespace testing;

bool set_env_var(const std::string &name, const std::string &value)
{
#ifdef _WIN32
    return _putenv_s(name.c_str(), value.c_str()) == 0;
#else
    return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

bool unset_env_var(const std::string &name)
{
#ifdef _WIN32
    return _putenv_s(name.c_str(), "") == 0;
#else
    return unsetenv(name.c_str()) == 0;
#endif
}

TEST(cli_subcommand_tests, list_and_power_together)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list", (char *)"power"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, 109);
    ASSERT_EQ(stdout, "");
    ASSERT_EQ(stderr,
        "The following argument was not expected: power\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, power_and_list_together)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, 109);
    ASSERT_EQ(stdout, "");
    ASSERT_EQ(stderr,
        "The following argument was not expected: list\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, list_no_devices)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(stdout, "");
    ASSERT_EQ(stderr, "No devices found.\n");
}

TEST(cli_subcommand_tests, list_test_devices)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(stdout, "Available devices:\n1. Test Device (TEST DEVICE, TEST_SERIAL_NUMBER, "
                      "located at TEST_ADDRESS)\n");
    ASSERT_EQ(stderr, "");
}

TEST(cli_subcommand_tests, list_random_subcommand)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list", (char *)"random"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, 109);
    ASSERT_EQ(stdout, "");
    ASSERT_EQ(stderr,
        "The following argument was not expected: random\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, test_power_both_access_flags)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-at-index", (char *)"0", (char *)"--device-with-serial", (char *)"TEST"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, 108);
    ASSERT_EQ(stdout, "");
    ASSERT_EQ(stderr,
        "--device-at-index excludes --device-with-serial\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, test_power_no_access_flags)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(stdout, "");
    ASSERT_EQ(stderr, "[Option Group: --device-at-index or --device-with-serial] is required.\nRun "
                      "with --help for more information.\n");
}

TEST(cli_subcommand_tests, test_power_status_no_device)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-with-serial", (char *)"TEST2"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(stdout, "");
    ASSERT_EQ(stderr, "No device was found.\n");
}

TEST(cli_subcommand_tests, test_power_status_via_index)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"1"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(stdout,
        "Test Device (TEST DEVICE, TEST_SERIAL_NUMBER, located at TEST_ADDRESS)\n  Socket 1: "
        "Unnamed socket, status: off\n");
    ASSERT_EQ(stderr, "");
}

TEST(cli_subcommand_tests, test_power_status_via_serial)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-with-serial", (char *)"TEST_SERIAL_NUMBER", (char *)"--sockets",
        (char *)"1"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(stdout,
        "Test Device (TEST DEVICE, TEST_SERIAL_NUMBER, located at TEST_ADDRESS)\n  Socket 1: "
        "Unnamed socket, status: off\n");
    ASSERT_EQ(stderr, "");
}

TEST(cli_subcommand_tests, test_power_on_specified_socket)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"on",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"1"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(stdout, "Test Device (TEST DEVICE, TEST_SERIAL_NUMBER, located at TEST_ADDRESS)\n  "
                      "Socket 1: turned on.\n");
    ASSERT_EQ(stderr, "");
}

TEST(cli_subcommand_tests, test_power_off_specified_socket)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"off",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"1"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(stdout, "Test Device (TEST DEVICE, TEST_SERIAL_NUMBER, located at TEST_ADDRESS)\n  "
                      "Socket 1: turned off.\n");
    ASSERT_EQ(stderr, "");
}

TEST(cli_subcommand_tests, test_power_toggle_specified_socket)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"1"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(stdout, "Test Device (TEST DEVICE, TEST_SERIAL_NUMBER, located at TEST_ADDRESS)\n  "
                      "Socket 1: toggled.\n");
    ASSERT_EQ(stderr, "");
}

TEST(cli_subcommand_tests, test_power_status_all)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"status",
        (char *)"--device-at-index", (char *)"0"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(stdout,
        "Test Device (TEST DEVICE, TEST_SERIAL_NUMBER, located at TEST_ADDRESS)\n  Socket 1: "
        "Unnamed socket, status: off\n  Socket 2: Unnamed socket, status: off\n  Socket 3: "
        "Unnamed socket, status: off\n  Socket 4: Unnamed socket, status: off\n");
    ASSERT_EQ(stderr, "");
}

TEST(cli_subcommand_tests, test_power_on_all)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"on",
        (char *)"--device-at-index", (char *)"0"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(stdout,
        "Test Device (TEST DEVICE, TEST_SERIAL_NUMBER, located at TEST_ADDRESS)\n  Socket 1: "
        "turned on.\n  Socket 2: turned on.\n  Socket 3: turned on.\n  Socket 4: turned on.\n");
    ASSERT_EQ(stderr, "");
}

TEST(cli_subcommand_tests, test_power_off_all)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"off",
        (char *)"--device-at-index", (char *)"0"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(stdout,
        "Test Device (TEST DEVICE, TEST_SERIAL_NUMBER, located at TEST_ADDRESS)\n  Socket 1: "
        "turned off.\n  Socket 2: turned off.\n  Socket 3: turned off.\n  Socket 4: turned off.\n");
    ASSERT_EQ(stderr, "");
}

TEST(cli_subcommand_tests, test_power_toggle_all)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"0"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(stdout,
        "Test Device (TEST DEVICE, TEST_SERIAL_NUMBER, located at TEST_ADDRESS)\n  Socket 1: "
        "toggled.\n  Socket 2: toggled.\n  Socket 3: toggled.\n  Socket 4: toggled.\n");
    ASSERT_EQ(stderr, "");
}

TEST(cli_subcommand_tests, test_power_too_big_socket_index)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"99"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(stdout, "Test Device (TEST DEVICE, TEST_SERIAL_NUMBER, located at TEST_ADDRESS)\n");
    ASSERT_EQ(stderr, "Socket index 99 is out of range.\n");
}

TEST(cli_subcommand_tests, test_power_zero_socket_index)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"0"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(stdout, "Test Device (TEST DEVICE, TEST_SERIAL_NUMBER, located at TEST_ADDRESS)\n");
    ASSERT_EQ(stderr, "Socket index 0 is out of range.\n");
}

TEST(cli_subcommand_tests, test_power_negative_socket_index)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"toggle",
        (char *)"--device-at-index", (char *)"0", (char *)"--sockets", (char *)"-1"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, 104);
    ASSERT_EQ(stdout, "");
    ASSERT_EQ(stderr, "Could not convert: --sockets = -1\nRun with --help for more information.\n");
}

TEST(cli_subcommand_tests, test_power_random_subcommand)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"random"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, 109);
    ASSERT_EQ(stdout, "");
    ASSERT_EQ(stderr, "The following argument was not expected: random\nRun with --help for more "
                      "information.\n");
}

TEST(cli_subcommand_tests, test_power_on_random_subcommand)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"on", (char *)"random"};

    ASSERT_TRUE(set_env_var("LIBSOKKETTER_TESTING_ENABLED", "1"));

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(unset_env_var("LIBSOKKETTER_TESTING_ENABLED"));

    ASSERT_EQ(return_code, 109);
    ASSERT_EQ(stdout, "");
    ASSERT_EQ(stderr, "The following argument was not expected: random\nRun with --help for more "
                      "information.\n");
}
