#include "cli_parser.h"
#include "libsokketter.h"

#include <fstream>
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
     * @brief points the update check cache at a temp file and stubs the real GitHub request,
     * so the notification branch of parse_and_process() is deterministic and offline.
     */
    class update_notification_test : public testing::Test
    {
    protected:
        auto SetUp() -> void override
        {
            m_cache_path =
                std::filesystem::temp_directory_path() / "sokketter-update-check-test.json";
            std::filesystem::remove(m_cache_path);

            set_env("LIBSOKKETTER_TEST_UPDATE_CHECK_PATH", m_cache_path.string().c_str());
            set_env("LIBSOKKETTER_TEST_SKIP_UPDATE_CHECK", "1");
            set_env("LIBSOKKETTER_TEST_DEVICE_NUMBER", "1");
        }

        auto TearDown() -> void override
        {
            unset_env("LIBSOKKETTER_TEST_UPDATE_CHECK_PATH");
            unset_env("LIBSOKKETTER_TEST_SKIP_UPDATE_CHECK");
            unset_env("LIBSOKKETTER_TEST_DEVICE_NUMBER");
            std::filesystem::remove(m_cache_path);
        }

        auto write_cache(const std::string &new_version) const -> void
        {
            std::ofstream file(m_cache_path);
            file << "{\"checked_at\":\"2026-01-01 00:00:00\",\"new_version\":\"" << new_version
                 << "\",\"latest_version\":\"" << new_version << "\"}";
        }

        std::filesystem::path m_cache_path;
    };
} // namespace

TEST_F(update_notification_test, populated_cache_prints_notification)
{
    write_cache("255.255.255.255");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(err, "A new sokketter version 255.255.255.255 is available at " +
                       sokketter::release_link() + ".\n");
}

TEST_F(update_notification_test, empty_cache_prints_nothing)
{
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(args.size(), args.data());

    testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(err, "");
}
