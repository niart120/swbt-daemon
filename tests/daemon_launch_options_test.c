#include "daemon/launch_options.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int expect_eq_int(int actual, int expected, const char *label) {
    (void)label;
    return actual == expected ? 0 : 1;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): test helper names arguments at call sites.
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

static int expect_false(int actual, const char *label) {
    (void)label;
    return actual == 0 ? 0 : 1;
}

static int expect_true(int actual, const char *label) {
    (void)label;
    return actual != 0 ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected, const char *label) {
    (void)label;
    return actual == expected ? 0 : 1;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): test fixture helper names arguments.
static int write_text_file(const char *path, const char *contents) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fputs(contents, file) < 0 || fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }
    return 0;
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

static int link_key_db_path_is_accepted(void) {
    const char *argv[] = {"swbt-daemon", "--link-key-db", "tmp/swbt-bond.tlv"};
    swbt_daemon_launch_options_t options = {0};
    swbt_daemon_launch_config_t launch_config = {0};
    const swbt_daemon_config_env_t env = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 3, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed += expect_str_eq(options.link_key_db_path, "tmp/swbt-bond.tlv", "link key db path");
    failed +=
        expect_true(swbt_daemon_launch_config_prepare(&launch_config, &options, &env), "prepare");
    failed += expect_true(launch_config.link_key_db_configured, "link key db configured");
    failed += expect_str_eq(launch_config.link_key_db_path, "tmp/swbt-bond.tlv",
                            "launch config link key db path");
    return failed;
}

static int link_key_db_equals_argument_is_accepted(void) {
    const char *argv[] = {"swbt-daemon", "--link-key-db=tmp/swbt-bond.tlv"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 2, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed += expect_str_eq(options.link_key_db_path, "tmp/swbt-bond.tlv", "link key db path");
    return failed;
}

static int backend_noop_separate_argument_is_accepted(void) {
    const char *argv[] = {"swbt-daemon", "--backend", "noop"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 3, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed += expect_eq_int((int)options.backend, (int)SWBT_DAEMON_LAUNCH_BACKEND_NOOP, "backend");
    return failed;
}

static int backend_noop_equals_argument_is_accepted(void) {
    const char *argv[] = {"swbt-daemon", "--backend=noop"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 2, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed += expect_eq_int((int)options.backend, (int)SWBT_DAEMON_LAUNCH_BACKEND_NOOP, "backend");
    return failed;
}

static int trace_path_separate_argument_is_accepted(void) {
    const char *argv[] = {"swbt-daemon", "--trace-path", "tmp/startup-trace.txt"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 3, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed += expect_str_eq(options.trace_path, "tmp/startup-trace.txt", "trace path");
    return failed;
}

static int trace_path_equals_argument_is_accepted(void) {
    const char *argv[] = {"swbt-daemon", "--trace-path=tmp/startup-trace.txt"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 2, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed += expect_str_eq(options.trace_path, "tmp/startup-trace.txt", "trace path");
    return failed;
}

static int missing_link_key_db_path_is_rejected(void) {
    const char *argv[] = {"swbt-daemon", "--link-key-db"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 2, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE, "parse");
    failed += expect_null(options.link_key_db_path, "link key db path");
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

static int invalid_backend_value_is_rejected(void) {
    const char *argv[] = {"swbt-daemon", "--backend", "dry-run"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 3, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_INVALID_ARGUMENT, "parse");
    return failed;
}

static int missing_backend_value_is_rejected(void) {
    const char *argv[] = {"swbt-daemon", "--backend"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 2, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE, "parse");
    return failed;
}

static int no_options_keeps_config_path_unset(void) {
    const char *argv[] = {"swbt-daemon"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 1, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed += expect_null(options.config_path, "config path");
    failed += expect_null(options.link_key_db_path, "link key db path");
    return failed;
}

static int no_options_defaults_to_production_backend(void) {
    const char *argv[] = {"swbt-daemon"};
    swbt_daemon_launch_options_t options = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 1, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed +=
        expect_eq_int((int)options.backend, (int)SWBT_DAEMON_LAUNCH_BACKEND_PRODUCTION, "backend");
    return failed;
}

static int launch_config_applies_file_before_env_and_sets_learned_target(void) {
    const char *path = "daemon-launch-options-config.toml";
    const char *argv[] = {"swbt-daemon", "--config", path};
    const swbt_daemon_config_env_t env = {
        .report_period_us = "15000",
    };
    swbt_daemon_launch_options_t options = {0};
    swbt_daemon_launch_config_t launch_config = {0};

    if (write_text_file(path, "[report]\n"
                              "period_us = 8000\n"
                              "\n"
                              "[active_reconnect]\n"
                              "switch_address = \"AA:BB:CC:DD:EE:FF\"\n") != 0) {
        return 1;
    }

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 3, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed +=
        expect_true(swbt_daemon_launch_config_prepare(&launch_config, &options, &env), "prepare");
    failed += expect_eq_u32(launch_config.config.report_period_us, 15000u, "env override");
    failed +=
        expect_str_eq(swbt_daemon_config_effective_reconnect_switch_address(&launch_config.config),
                      "AA:BB:CC:DD:EE:FF", "effective address");
    failed += expect_true(launch_config.learned_switch_address_target_configured, "target");
    failed += expect_str_eq(launch_config.learned_switch_address_target.path, path, "target path");
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int launch_config_without_config_path_keeps_learned_target_unset(void) {
    const char *argv[] = {"swbt-daemon"};
    const swbt_daemon_config_env_t env = {
        .report_period_us = "15000",
    };
    swbt_daemon_launch_options_t options = {0};
    swbt_daemon_launch_config_t launch_config = {0};

    int failed = 0;
    failed += expect_eq_int((int)swbt_daemon_launch_options_parse(&options, 1, argv),
                            (int)SWBT_DAEMON_LAUNCH_OPTIONS_OK, "parse");
    failed +=
        expect_true(swbt_daemon_launch_config_prepare(&launch_config, &options, &env), "prepare");
    failed += expect_eq_u32(launch_config.config.report_period_us, 15000u, "env override");
    failed += expect_false(launch_config.learned_switch_address_target_configured, "target");
    failed += expect_null(launch_config.learned_switch_address_target.path, "target path");
    return failed;
}

int main(void) {
    int failed = 0;
    failed += config_path_separate_argument_is_accepted();
    failed += config_path_equals_argument_is_accepted();
    failed += link_key_db_path_is_accepted();
    failed += link_key_db_equals_argument_is_accepted();
    failed += backend_noop_separate_argument_is_accepted();
    failed += backend_noop_equals_argument_is_accepted();
    failed += trace_path_separate_argument_is_accepted();
    failed += trace_path_equals_argument_is_accepted();
    failed += missing_link_key_db_path_is_rejected();
    failed += missing_config_path_is_rejected();
    failed += unknown_option_is_rejected();
    failed += invalid_backend_value_is_rejected();
    failed += missing_backend_value_is_rejected();
    failed += no_options_keeps_config_path_unset();
    failed += no_options_defaults_to_production_backend();
    failed += launch_config_applies_file_before_env_and_sets_learned_target();
    failed += launch_config_without_config_path_keeps_learned_target_unset();
    return failed == 0 ? 0 : 1;
}
