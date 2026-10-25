#include "cli_parser.h"
#include "libsokketter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

using namespace testing;

namespace {
    auto set_test_env(const char *name, const char *value) -> void
    {
#ifdef _WIN32
        _putenv_s(name, value);
#else
        setenv(name, value, 1);
#endif
    }

    auto unset_test_env(const char *name) -> void
    {
#ifdef _WIN32
        _putenv_s(name, "");
#else
        unsetenv(name);
#endif
    }

    /**
     * @brief mirrors the CLI's default display filter: fake test devices are always visible, so
     * with LIBSOKKETTER_TEST_DEVICE_NUMBER set this returns the first fake device.
     */
    std::shared_ptr<sokketter::power_strip> first_available_device()
    {
        sokketter::device_filter filter;
        filter.included_types = sokketter::power_strip_type::USB_DEVICES;

        for (const auto &device : sokketter::devices(filter))
        {
            return device;
        }

        return nullptr;
    }

    /**
     * @brief the device header line the login and logout subcommands print before acting,
     * mirroring the CLI's "std::cout << device->to_string() << std::endl;".
     */
    std::string expected_device_header(const std::shared_ptr<sokketter::power_strip> &device)
    {
        if (device == nullptr)
        {
            return "";
        }

        return device->to_string() + "\n";
    }

    /**
     * @brief RAII scoping of the fake-device environment variables so every authentication test
     * controls both the device count and the opt-in fake authentication password, and cleans up
     * after itself regardless of assertion outcomes.
     */
    class fake_authentication_scope
    {
    public:
        fake_authentication_scope(const char *device_number, const char *authentication_password)
        {
            set_test_env("LIBSOKKETTER_TEST_DEVICE_NUMBER", device_number);

            if (authentication_password != nullptr)
            {
                set_test_env("LIBSOKKETTER_TEST_DEVICE_PASSWORD", authentication_password);
            }
            else
            {
                unset_test_env("LIBSOKKETTER_TEST_DEVICE_PASSWORD");
            }
        }

        ~fake_authentication_scope()
        {
            unset_test_env("LIBSOKKETTER_TEST_DEVICE_NUMBER");
            unset_test_env("LIBSOKKETTER_TEST_DEVICE_PASSWORD");
        }

        fake_authentication_scope(const fake_authentication_scope &) = delete;
        auto operator=(const fake_authentication_scope &) -> fake_authentication_scope & = delete;
    };
} // namespace

TEST(cli_authentication_tests, login_without_device_group)
{
    // MAN-CLI-30
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"login",
        (char *)"--authentication-type", (char *)"password", (char *)"--password",
        (char *)"secret"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "[Option Group: --device-at-index or --device-with-serial] is required.\nRun "
                   "with --help for more information.\n");
}

TEST(cli_authentication_tests, logout_without_device_group)
{
    // MAN-CLI-31
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"logout"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "[Option Group: --device-at-index or --device-with-serial] is required.\nRun "
                   "with --help for more information.\n");
}

TEST(cli_authentication_tests, login_with_both_access_flags_is_rejected)
{
    // MAN-CLI-32
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"login",
        (char *)"--device-at-index", (char *)"1", (char *)"--device-with-serial", (char *)"TEST",
        (char *)"--authentication-type", (char *)"password", (char *)"--password",
        (char *)"secret"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err,
        "--device-at-index excludes --device-with-serial\nRun with --help for more information.\n");
}

TEST(cli_authentication_tests, login_without_authentication_type)
{
    // MAN-CLI-33
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"login",
        (char *)"--device-at-index", (char *)"1", (char *)"--password", (char *)"secret"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "The --authentication-type option is required for the login subcommand. "
                   "Available types are: PASSWORD.\n");
}

TEST(cli_authentication_tests, login_with_unknown_authentication_type)
{
    // MAN-CLI-34
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"login",
        (char *)"--device-at-index", (char *)"1", (char *)"--authentication-type",
        (char *)"certificate", (char *)"--password", (char *)"secret"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "Unknown authentication type: certificate. Available types are: PASSWORD.\n");
}

TEST(cli_authentication_tests, login_password_type_without_password_option)
{
    // MAN-CLI-35
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"login",
        (char *)"--device-at-index", (char *)"1", (char *)"--authentication-type",
        (char *)"PASSWORD"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "The --password option is required for the PASSWORD authentication type.\n");
}

TEST(cli_authentication_tests, login_on_device_without_authentication_is_rejected)
{
    // MAN-CLI-36
    /**
     * @attention fake devices without the opt-in LIBSOKKETTER_TEST_DEVICE_PASSWORD advertise no
     * authentication mechanism, so login must reject them before attempting any authentication.
     */
    const fake_authentication_scope scope("1", nullptr);

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"login",
        (char *)"--device-at-index", (char *)"1", (char *)"--authentication-type",
        (char *)"password", (char *)"--password", (char *)"secret"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "Device does not require authentication.\n");
}

TEST(cli_authentication_tests, logout_on_device_without_authentication_is_rejected)
{
    // MAN-CLI-37
    const fake_authentication_scope scope("1", nullptr);

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"logout",
        (char *)"--device-at-index", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "Device does not require authentication.\n");
}

TEST(cli_authentication_tests, login_with_wrong_password_fails)
{
    // MAN-CLI-38
    const fake_authentication_scope scope("1", "correct horse battery staple");

    const auto device = first_available_device();
    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"login",
        (char *)"--device-at-index", (char *)"1", (char *)"--authentication-type",
        (char *)"password", (char *)"--password", (char *)"wrong password"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, expected_device_header(device));
    ASSERT_EQ(err, "Authentication failed.\n");
}

TEST(cli_authentication_tests, login_with_correct_password_succeeds)
{
    // MAN-CLI-39
    const fake_authentication_scope scope("1", "correct horse battery staple");

    const auto device = first_available_device();
    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"login",
        (char *)"--device-at-index", (char *)"1", (char *)"--authentication-type",
        (char *)"PASSWORD", (char *)"--password", (char *)"correct horse battery staple"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_device_header(device) + "Authenticated successfully.\n");
    ASSERT_EQ(err, "");
}

TEST(cli_authentication_tests, login_via_serial_succeeds)
{
    // MAN-CLI-40
    const fake_authentication_scope scope("1", "secret");

    const auto device = first_available_device();
    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"login",
        (char *)"--device-with-serial", (char *)"TEST_SERIAL_NUMBER_0",
        (char *)"--authentication-type", (char *)"password", (char *)"--password",
        (char *)"secret"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_device_header(device) + "Authenticated successfully.\n");
    ASSERT_EQ(err, "");
}

/**
 * @attention a fake device with the opt-in LIBSOKKETTER_TEST_DEVICE_PASSWORD advertises
 * PASSWORD_ONLY but carries no stored credential in its configuration, so logout must report that
 * there is nothing to remove instead of falsely claiming a revocation.
 */
TEST(cli_authentication_tests, logout_on_device_without_stored_credentials_is_rejected)
{
    // MAN-CLI-41
    const fake_authentication_scope scope("1", "secret");

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"logout",
        (char *)"--device-at-index", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "No credentials are stored for this device.\n");
}

TEST(cli_authentication_tests, login_help_takes_precedence)
{
    // MAN-CLI-42
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"login", (char *)"--help"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(out, HasSubstr("A command-line interface for controlling attached power strips"));
    ASSERT_EQ(err, "");
}

TEST(cli_authentication_tests, logout_help_takes_precedence)
{
    // MAN-CLI-43
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"logout", (char *)"/help"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_THAT(out, HasSubstr("A command-line interface for controlling attached power strips"));
    ASSERT_EQ(err, "");
}

TEST(cli_authentication_tests, login_windows_style_underscored_options)
{
    // MAN-CLI-44
    const fake_authentication_scope scope("1", "secret");

    const auto device = first_available_device();
    if (device == nullptr)
    {
        GTEST_SKIP() << "no device is available";
    }

    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"login",
        (char *)"/device_at_index", (char *)"1", (char *)"/authentication_type",
        (char *)"PASSWORD", (char *)"/password", (char *)"secret"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_SUCCESS);
    ASSERT_EQ(out, expected_device_header(device) + "Authenticated successfully.\n");
    ASSERT_EQ(err, "");
}

TEST(cli_authentication_tests, logout_does_not_accept_authentication_options)
{
    // MAN-CLI-45
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"logout",
        (char *)"--device-at-index", (char *)"1", (char *)"--password", (char *)"secret"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "The following arguments were not expected: secret --password\nRun with --help "
                   "for more information.\n");
}

TEST(cli_authentication_tests, login_and_list_together_is_rejected)
{
    // MAN-CLI-46
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"login",
        (char *)"--device-at-index", (char *)"1", (char *)"--authentication-type",
        (char *)"password", (char *)"--password", (char *)"secret", (char *)"list"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_THAT(err, HasSubstr("The following argument was not expected: list"));
}

TEST(cli_authentication_tests, power_with_login_subcommand_is_rejected)
{
    // MAN-CLI-47
    std::vector<char *> args = {(char *)"sokketter-cli", (char *)"power", (char *)"login",
        (char *)"--device-at-index", (char *)"1"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const auto &return_code = cli_parser::parse_and_process(
        static_cast<int>(args.size()), args.data());

    const auto &out = testing::internal::GetCapturedStdout();
    const auto &err = testing::internal::GetCapturedStderr();

    ASSERT_EQ(return_code, EXIT_FAILURE);
    ASSERT_EQ(out, "");
    ASSERT_EQ(err, "A subcommand is required\nRun with --help for more information.\n");
}
