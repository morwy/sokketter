#include "cli_parser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

using namespace testing;

TEST(cli_general_tests, no_args)
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
