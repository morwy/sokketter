#include "cli_parser.h"
#include "libsokketter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

using namespace testing;

namespace
{
class cli_test_environment final : public testing::Environment
{
public:
    auto SetUp() -> void override
    {
        ASSERT_TRUE(sokketter::initialize());
    }

    auto TearDown() -> void override
    {
        ASSERT_TRUE(sokketter::deinitialize());
    }
};

const auto *cli_tests_environment =
    testing::AddGlobalTestEnvironment(new cli_test_environment());
} // namespace

TEST(cli_general_tests, no_args)
{
    std::vector<char *> args = {(char *)"sokketter-cli"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, 106);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "A subcommand is required\nRun with --help for more information.\n");
}
