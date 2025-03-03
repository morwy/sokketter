#include "cli_parser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

using namespace testing;

TEST(cli_option_tests, help_long_flag)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"--help"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(
        stdout, ContainsRegex(
                    "A command-line interface for controlling attached power strips and sockets."));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, help_short_flag)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"-h"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(
        stdout, ContainsRegex(
                    "A command-line interface for controlling attached power strips and sockets."));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, version_long_flag)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"--version"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(stdout, ContainsRegex("sokketter-cli version "));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, version_short_flag)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"-v"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(stdout, ContainsRegex("sokketter-cli version "));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, help_precedence_over_list_1)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list", (char *)"--help"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(
        stdout, ContainsRegex(
                    "A command-line interface for controlling attached power strips and sockets."));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, help_precedence_over_list_2)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"--help", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(
        stdout, ContainsRegex(
                    "A command-line interface for controlling attached power strips and sockets."));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, help_precedence_over_power_1)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"--help"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(
        stdout, ContainsRegex(
                    "A command-line interface for controlling attached power strips and sockets."));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, help_precedence_over_power_2)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"--help", (char *)"power"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(
        stdout, ContainsRegex(
                    "A command-line interface for controlling attached power strips and sockets."));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, help_precedence_over_power_on)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"on", (char *)"--help"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(
        stdout, ContainsRegex(
                    "A command-line interface for controlling attached power strips and sockets."));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, help_precedence_over_power_off)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"off", (char *)"--help"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(
        stdout, ContainsRegex(
                    "A command-line interface for controlling attached power strips and sockets."));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, help_precedence_over_power_toggle)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"toggle", (char *)"--help"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(
        stdout, ContainsRegex(
                    "A command-line interface for controlling attached power strips and sockets."));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, help_precedence_over_power_status)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"status", (char *)"--help"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(
        stdout, ContainsRegex(
                    "A command-line interface for controlling attached power strips and sockets."));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, version_precedence_over_list_1)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list", (char *)"--version"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(stdout, ContainsRegex("sokketter-cli version "));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, version_precedence_over_list_2)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"--version", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(stdout, ContainsRegex("sokketter-cli version "));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, version_precedence_over_power_1)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"--version"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(stdout, ContainsRegex("sokketter-cli version "));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, version_precedence_over_power_2)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"--version", (char *)"power"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(stdout, ContainsRegex("sokketter-cli version "));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, version_precedence_over_power_on)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"on", (char *)"--version"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(stdout, ContainsRegex("sokketter-cli version "));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, version_precedence_over_power_off)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"off", (char *)"--version"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(stdout, ContainsRegex("sokketter-cli version "));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, version_precedence_over_power_toggle)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"toggle", (char *)"--version"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(stdout, ContainsRegex("sokketter-cli version "));
    ASSERT_EQ(stderr, "");
}

TEST(cli_option_tests, version_precedence_over_power_status)
{
    std::vector<char *> args = {
        (char *)"sokketter-cli", (char *)"power", (char *)"status", (char *)"--version"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(stdout, ContainsRegex("sokketter-cli version "));
    ASSERT_EQ(stderr, "");
}
