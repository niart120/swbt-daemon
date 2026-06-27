#include "daemon/cli.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "daemon/launch_options.h"

static void swbt_daemon_cli_write_help(FILE *out) {
    if (out == NULL) {
        return;
    }
    fputs("Usage:\n", out);
    fputs("  swbt-daemon [options]\n", out);
    fputs("  swbt-daemon <command> [options]\n", out);
    fputs("\n", out);
    fputs("Commands:\n", out);
    fputs("  help      Show command and option help.\n", out);
    fputs("  adapters  List adapter-location selector candidates.\n", out);
    fputs("  config    Show effective launch and runtime config.\n", out);
    fputs("\n", out);
    fputs("Startup options:\n", out);
    fputs("  --backend production|noop\n", out);
    fputs("  --config <path>\n", out);
    fputs("  --link-key-db <path>\n", out);
    fputs("  --trace-path <path>\n", out);
    fputs("  --hci-dump-path <path>\n", out);
    fputs("  --crash-dump-path <path>\n", out);
    fputs("  --adapter-location <selector>\n", out);
}

static bool swbt_daemon_cli_is_help_command(const char *argument) {
    return argument != NULL && (strcmp(argument, "help") == 0 || strcmp(argument, "--help") == 0 ||
                                strcmp(argument, "-h") == 0);
}

static bool swbt_daemon_cli_is_command(const char *argument, const char *command) {
    return argument != NULL && command != NULL && strcmp(argument, command) == 0;
}

static void swbt_daemon_cli_write_error(FILE *err, const char *message) {
    if (err == NULL || message == NULL) {
        return;
    }
    fputs(message, err);
    fputc('\n', err);
}

static const char *swbt_daemon_cli_value_or_none(const char *value) {
    return value != NULL && value[0] != '\0' ? value : "<none>";
}

static const char *swbt_daemon_cli_backend_string(swbt_daemon_launch_backend_t backend) {
    switch (backend) {
    case SWBT_DAEMON_LAUNCH_BACKEND_PRODUCTION:
        return "production";
    case SWBT_DAEMON_LAUNCH_BACKEND_NOOP:
        return "noop";
    }
    return "unknown";
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): key/value output pair.
static void swbt_daemon_cli_write_key_value(FILE *out, const char *key, const char *value) {
    if (out == NULL || key == NULL) {
        return;
    }
    fputs(key, out);
    fputc('=', out);
    fputs(swbt_daemon_cli_value_or_none(value), out);
    fputc('\n', out);
}

static void swbt_daemon_cli_write_key_u32(FILE *out, const char *key, uint32_t value) {
    char buffer[16];

    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    if (snprintf(buffer, sizeof(buffer), "%" PRIu32, value) < 0) {
        return;
    }
    swbt_daemon_cli_write_key_value(out, key, buffer);
}

static void swbt_daemon_cli_write_key_uint(FILE *out, const char *key, unsigned int value) {
    char buffer[16];

    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    if (snprintf(buffer, sizeof(buffer), "%u", value) < 0) {
        return;
    }
    swbt_daemon_cli_write_key_value(out, key, buffer);
}

static void swbt_daemon_cli_write_key_int(FILE *out, const char *key, int value) {
    char buffer[16];

    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    if (snprintf(buffer, sizeof(buffer), "%d", value) < 0) {
        return;
    }
    swbt_daemon_cli_write_key_value(out, key, buffer);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): CLI stream pair.
static int swbt_daemon_cli_run_adapters(int argc, FILE *out, FILE *err,
                                        const swbt_daemon_cli_ports_t *ports) {
    if (argc != 2) {
        swbt_daemon_cli_write_error(err, "adapters does not accept arguments");
        return 1;
    }
    if (ports == NULL || ports->list_adapters == NULL) {
        swbt_daemon_cli_write_error(err, "adapter inventory is not available");
        return 1;
    }
    return ports->list_adapters(ports->adapter_inventory_context, out, err) == 0 ? 0 : 1;
}

static void swbt_daemon_cli_write_config(FILE *out, const swbt_daemon_launch_options_t *options,
                                         const swbt_daemon_launch_config_t *launch_config) {
    const swbt_daemon_config_t *config = NULL;
    const char *effective_switch_address = NULL;

    if (out == NULL || options == NULL || launch_config == NULL) {
        return;
    }

    config = &launch_config->config;
    effective_switch_address = swbt_daemon_config_effective_reconnect_switch_address(config);

    swbt_daemon_cli_write_key_value(out, "backend",
                                    swbt_daemon_cli_backend_string(options->backend));
    swbt_daemon_cli_write_key_value(out, "config.path", options->config_path);
    swbt_daemon_cli_write_key_value(
        out, "link_key_db.path",
        launch_config->link_key_db_configured ? launch_config->link_key_db_path : NULL);
    swbt_daemon_cli_write_key_value(out, "trace.path", options->trace_path);
    swbt_daemon_cli_write_key_value(out, "hci_dump.path", launch_config->hci_dump_path);
    swbt_daemon_cli_write_key_value(out, "crash_dump.path", options->crash_dump_path);
    swbt_daemon_cli_write_key_value(
        out, "adapter_location",
        launch_config->adapter_location_configured ? launch_config->adapter_location : NULL);
    swbt_daemon_cli_write_key_u32(out, "runtime.report_period_us", config->report_period_us);
    swbt_daemon_cli_write_key_value(out, "runtime.ipc.host", config->ipc_host);
    swbt_daemon_cli_write_key_uint(out, "runtime.ipc.port", (unsigned int)config->ipc_port);
    swbt_daemon_cli_write_key_int(out, "runtime.ipc.backlog", config->ipc_backlog);
    swbt_daemon_cli_write_key_u32(out, "runtime.ipc.heartbeat_timeout_ms",
                                  config->ipc_heartbeat_timeout_ms);
    swbt_daemon_cli_write_key_value(out, "active_reconnect.explicit_switch_address",
                                    config->active_reconnect_explicit_switch_address);
    swbt_daemon_cli_write_key_value(out, "active_reconnect.learned_switch_address",
                                    config->active_reconnect_learned_switch_address);
    swbt_daemon_cli_write_key_value(out, "active_reconnect.effective_switch_address",
                                    effective_switch_address);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): CLI stream pair.
static int swbt_daemon_cli_run_config(int argc, const char *const argv[], FILE *out, FILE *err,
                                      const swbt_daemon_cli_ports_t *ports) {
    swbt_daemon_launch_options_t options;
    swbt_daemon_launch_config_t launch_config;
    const swbt_daemon_config_env_t empty_env = {0};
    const swbt_daemon_config_env_t *env =
        ports != NULL && ports->config_env != NULL ? ports->config_env : &empty_env;

    if (swbt_daemon_launch_options_parse(&options, argc - 1, argv + 1) !=
        SWBT_DAEMON_LAUNCH_OPTIONS_OK) {
        swbt_daemon_cli_write_error(err, "config options are invalid");
        return 1;
    }
    if (!swbt_daemon_launch_config_prepare(&launch_config, &options, env)) {
        swbt_daemon_cli_write_error(err, "config is invalid");
        return 1;
    }
    swbt_daemon_cli_write_config(out, &options, &launch_config);
    return 0;
}

swbt_daemon_cli_dispatch_result_t
swbt_daemon_cli_dispatch_with_ports(int argc, const char *const argv[], FILE *out, FILE *err,
                                    const swbt_daemon_cli_ports_t *ports) {
    int exit_code = 0;

    if (argc < 2 || argv == NULL || argv[1] == NULL) {
        return (swbt_daemon_cli_dispatch_result_t){.handled = false, .exit_code = 0};
    }
    if (swbt_daemon_cli_is_help_command(argv[1])) {
        swbt_daemon_cli_write_help(out);
        return (swbt_daemon_cli_dispatch_result_t){.handled = true, .exit_code = 0};
    }
    if (swbt_daemon_cli_is_command(argv[1], "adapters")) {
        exit_code = swbt_daemon_cli_run_adapters(argc, out, err, ports);
        return (swbt_daemon_cli_dispatch_result_t){.handled = true, .exit_code = exit_code};
    }
    if (swbt_daemon_cli_is_command(argv[1], "config")) {
        exit_code = swbt_daemon_cli_run_config(argc, argv, out, err, ports);
        return (swbt_daemon_cli_dispatch_result_t){.handled = true, .exit_code = exit_code};
    }
    return (swbt_daemon_cli_dispatch_result_t){.handled = false, .exit_code = 0};
}

swbt_daemon_cli_dispatch_result_t swbt_daemon_cli_dispatch(int argc, const char *const argv[],
                                                           FILE *out, FILE *err) {
    return swbt_daemon_cli_dispatch_with_ports(argc, argv, out, err, NULL);
}
