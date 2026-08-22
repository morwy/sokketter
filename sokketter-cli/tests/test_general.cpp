#include "cli_parser.h"
#include "libsokketter.h"

#include <filesystem>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

using namespace testing;

namespace {
    auto set_env(const char *name, const char *value) -> void
    {
#ifdef _WIN32
        _putenv_s(name, value);
#else
        setenv(name, value, 1);
#endif
    }

    auto unset_env(const char *name) -> void
    {
#ifdef _WIN32
        _putenv_s(name, "");
#else
        unsetenv(name);
#endif
    }

    /**
     * @brief without this, tests read the real, per-machine update-check cache and can start a real
     * GitHub request, so every subcommand test is at the mercy of the runner's persisted state.
     */
    auto default_update_check_cache_path() -> std::filesystem::path
    { return std::filesystem::temp_directory_path() / "sokketter-cli-tests-update-check.json"; }

    class cli_test_environment final : public testing::Environment
    {
    public:
        auto SetUp() -> void override
        {
            const auto &cache_path = default_update_check_cache_path();
            std::filesystem::remove(cache_path);
            set_env("LIBSOKKETTER_TEST_UPDATE_CHECK_PATH", cache_path.string().c_str());
            set_env("LIBSOKKETTER_TEST_SKIP_UPDATE_CHECK", "1");

            ASSERT_TRUE(sokketter::initialize());
        }

        auto TearDown() -> void override
        {
            ASSERT_TRUE(sokketter::deinitialize());

            unset_env("LIBSOKKETTER_TEST_UPDATE_CHECK_PATH");
            unset_env("LIBSOKKETTER_TEST_SKIP_UPDATE_CHECK");
            std::filesystem::remove(default_update_check_cache_path());
        }
    };

    const auto *cli_tests_environment =
        testing::AddGlobalTestEnvironment(new cli_test_environment());
} // namespace

TEST(cli_general_tests, no_args)
{
    // MAN-CLI-01
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
