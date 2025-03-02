#include "cli_parser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

using namespace testing;

TEST(sokketter_cli_tests, no_args)
{
    std::vector<char *> args = {(char *)"sokketter-cli"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &stdout = testing::internal::GetCapturedStdout();
    const auto &stderr = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, 106);
    ASSERT_EQ(stdout, "");
    ASSERT_EQ(stderr, "A subcommand is required\nRun with --help for more information.\n");
}

TEST(sokketter_cli_tests, display_help_long)
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

TEST(sokketter_cli_tests, display_help_short)
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

TEST(sokketter_cli_tests, display_version_long)
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

TEST(sokketter_cli_tests, display_version_short)
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
