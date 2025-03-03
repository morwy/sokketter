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

TEST(cli_subcommand_tests, test_power_status)
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
