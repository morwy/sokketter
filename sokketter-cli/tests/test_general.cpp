#include "cli_parser.h"
#include "libsokketter.h"

#include <filesystem>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#ifdef _WIN32
#    include <process.h>
#else
#    include <unistd.h>
#endif

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

    auto current_process_id() -> int
    {
#ifdef _WIN32
        return _getpid();
#else
        return getpid();
#endif
    }

    /**
     * @brief without this, tests read the real, per-machine update-check cache and can start a real
     * GitHub request, so every subcommand test is at the mercy of the runner's persisted state.
     */
    auto default_update_check_cache_path() -> std::filesystem::path
    { return std::filesystem::temp_directory_path() / "sokketter-cli-tests-update-check.json"; }

    /**
     * @brief login/logout success paths call device save() (and deinitialize() persists the
     * database), which writes devices.json under storage_path(); without redirecting the whole
     * storage tree the unit suite rewrites the developer's real application database
     * (C:\ProgramData\sokketter and its POSIX equivalents).
     * @attention the directory is made unique per process so parallel ctest runs (each discovered
     * TEST_F is its own process) never race on a shared devices.json.
     */
    auto default_test_storage_path() -> std::filesystem::path
    {
        return std::filesystem::temp_directory_path() /
            ("sokketter-cli-tests-storage-" + std::to_string(current_process_id()));
    }

    class cli_test_environment final : public testing::Environment
    {
    public:
        auto SetUp() -> void override
        {
            const auto &cache_path = default_update_check_cache_path();
            std::filesystem::remove(cache_path);
            set_env("LIBSOKKETTER_TEST_UPDATE_CHECK_PATH", cache_path.string().c_str());
            set_env("LIBSOKKETTER_TEST_SKIP_UPDATE_CHECK", "1");

            /**
             * @attention redirect before initialize(): the load() inside initialize() then reads
             * the throwaway database instead of the developer's, and every save() during the test
             * (login/logout) and the deinitialize() save() in TearDown() land there too.
             */
            const auto &storage_dir = default_test_storage_path();
            std::filesystem::remove_all(storage_dir);
            std::filesystem::create_directories(storage_dir);
            set_env("LIBSOKKETTER_TEST_STORAGE_PATH", storage_dir.string().c_str());

            ASSERT_TRUE(sokketter::initialize());
        }

        auto TearDown() -> void override
        {
            ASSERT_TRUE(sokketter::deinitialize());

            unset_env("LIBSOKKETTER_TEST_UPDATE_CHECK_PATH");
            unset_env("LIBSOKKETTER_TEST_SKIP_UPDATE_CHECK");
            std::filesystem::remove(default_update_check_cache_path());

            unset_env("LIBSOKKETTER_TEST_STORAGE_PATH");
            std::filesystem::remove_all(default_test_storage_path());
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

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "A subcommand is required\nRun with --help for more information.\n");
}
