#include "daemon/launch_options.h"

#include <stddef.h>
#include <string.h>

static int expect_eq_int(int actual, int expected, const char *label) {
    (void)label;
    return actual == expected ? 0 : 1;
}

static int expect_str_eq(const char *actual, const char *expected, const char *label) {
    (void)label;
    if (actual == NULL || expected == NULL) {
        return actual == expected ? 0 : 1;
    }
    return strcmp(actual, expected) == 0 ? 0 : 1;
}

static int expect_null(const void *actual, const char *label) {
    (void)label;
    return actual == NULL ? 0 : 1;
}

static int config_path_separate_argument_is_accepted(void) {
    const char *argv[] = {"swbt-daemon", "--config", "tmp/reconnect.toml"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 3, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed += expect_str_eq(options.config_path, "tmp/reconnect.toml", "config path");
    return failed;
}

static int config_path_equals_argument_is_accepted(void) {
    const char *argv[] = {"swbt-daemon", "--config=tmp/reconnect.toml"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 2, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed += expect_str_eq(options.config_path, "tmp/reconnect.toml", "config path");
    return failed;
}

static int missing_config_path_is_rejected(void) {
    const char *argv[] = {"swbt-daemon", "--config"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 2, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE, "parse");
    failed += expect_null(options.config_path, "config path");
    return failed;
}

static int unknown_option_is_rejected(void) {
    const char *argv[] = {"swbt-daemon", "--unknown"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 2, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_UNKNOWN_OPTION, "parse");
    failed += expect_null(options.config_path, "config path");
    return failed;
}

static int no_options_keeps_config_path_unset(void) {
    const char *argv[] = {"swbt-daemon"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 1, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed += expect_null(options.config_path, "config path");
    return failed;
}

int main(void) {
    int failed = 0;
    failed += config_path_separate_argument_is_accepted();
    failed += config_path_equals_argument_is_accepted();
    failed += missing_config_path_is_rejected();
    failed += unknown_option_is_rejected();
    failed += no_options_keeps_config_path_unset();
    return failed == 0 ? 0 : 1;
}
