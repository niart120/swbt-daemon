#include "daemon/cli.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    int calls;
} fake_adapter_inventory_t;

static int expect_true(int actual, const char *label) {
    (void)label;
    return actual != 0 ? 0 : 1;
}

static int expect_eq_int(int actual, int expected, const char *label) {
    (void)label;
    return actual == expected ? 0 : 1;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): test helper names arguments at call sites.
static int expect_contains(const char *text, const char *needle, const char *label) {
    (void)label;
    return text != NULL && strstr(text, needle) != NULL ? 0 : 1;
}

static int read_file(FILE *file, char *buffer, size_t buffer_size) {
    size_t read_size;
    if (file == NULL || buffer == NULL || buffer_size == 0u || fflush(file) != 0 ||
        fseek(file, 0L, SEEK_SET) != 0) {
        return 1;
    }
    read_size = fread(buffer, 1u, buffer_size - 1u, file);
    buffer[read_size] = '\0';
    return ferror(file) ? 1 : 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): callback ABI under test.
static int fake_list_adapters(void *context, FILE *out, FILE *err) {
    fake_adapter_inventory_t *inventory = (fake_adapter_inventory_t *)context;
    (void)err;

    if (inventory == NULL || out == NULL) {
        return 1;
    }
    ++inventory->calls;
    fputs("libusb:1:3.2\n", out);
    fputs("winusb:PCIROOT(0)#USBROOT(0)#USB(9)#USB(1)\n", out);
    return 0;
}

static int help_command_prints_management_surface(void) {
    const char *argv[] = {"swbt-daemon", "help"};
    char out_buffer[2048];
    FILE *out = tmpfile();
    FILE *err = tmpfile();
    swbt_daemon_cli_dispatch_result_t result = {0};
    int failed = 0;

    if (out == NULL || err == NULL) {
        if (out != NULL) {
            (void)fclose(out);
        }
        if (err != NULL) {
            (void)fclose(err);
        }
        return 1;
    }

    result = swbt_daemon_cli_dispatch(2, argv, out, err);
    failed += expect_true(result.handled, "handled");
    failed += expect_eq_int(result.exit_code, 0, "exit");
    failed += read_file(out, out_buffer, sizeof(out_buffer));
    failed += expect_contains(out_buffer, "Usage:", "usage");
    failed += expect_contains(out_buffer, "Commands:", "commands");
    failed += expect_contains(out_buffer, "help", "help command");
    failed += expect_contains(out_buffer, "adapters", "adapters command");
    failed += expect_contains(out_buffer, "config", "config command");
    failed += expect_contains(out_buffer, "--backend production|noop", "backend option");
    failed += expect_contains(out_buffer, "--adapter-location", "adapter option");

    failed += fclose(out) == 0 ? 0 : 1;
    failed += fclose(err) == 0 ? 0 : 1;
    return failed;
}

static int adapters_command_lists_adapter_location_candidates(void) {
    const char *argv[] = {"swbt-daemon", "adapters"};
    fake_adapter_inventory_t inventory = {0};
    const swbt_daemon_cli_ports_t ports = {
        .list_adapters = fake_list_adapters,
        .adapter_inventory_context = &inventory,
    };
    char out_buffer[2048];
    FILE *out = tmpfile();
    FILE *err = tmpfile();
    swbt_daemon_cli_dispatch_result_t result = {0};
    int failed = 0;

    if (out == NULL || err == NULL) {
        if (out != NULL) {
            (void)fclose(out);
        }
        if (err != NULL) {
            (void)fclose(err);
        }
        return 1;
    }

    result = swbt_daemon_cli_dispatch_with_ports(2, argv, out, err, &ports);
    failed += expect_true(result.handled, "handled");
    failed += expect_eq_int(result.exit_code, 0, "exit");
    failed += expect_eq_int(inventory.calls, 1, "inventory calls");
    failed += read_file(out, out_buffer, sizeof(out_buffer));
    failed += expect_contains(out_buffer, "libusb:1:3.2", "libusb selector");
    failed += expect_contains(out_buffer, "winusb:PCIROOT", "winusb selector");

    failed += fclose(out) == 0 ? 0 : 1;
    failed += fclose(err) == 0 ? 0 : 1;
    return failed;
}

static int adapters_command_fails_when_inventory_is_not_available(void) {
    const char *argv[] = {"swbt-daemon", "adapters"};
    char err_buffer[2048];
    FILE *out = tmpfile();
    FILE *err = tmpfile();
    swbt_daemon_cli_dispatch_result_t result = {0};
    int failed = 0;

    if (out == NULL || err == NULL) {
        if (out != NULL) {
            (void)fclose(out);
        }
        if (err != NULL) {
            (void)fclose(err);
        }
        return 1;
    }

    result = swbt_daemon_cli_dispatch(2, argv, out, err);
    failed += expect_true(result.handled, "handled");
    failed += expect_eq_int(result.exit_code, 1, "exit");
    failed += read_file(err, err_buffer, sizeof(err_buffer));
    failed += expect_contains(err_buffer, "adapter inventory is not available", "error");

    failed += fclose(out) == 0 ? 0 : 1;
    failed += fclose(err) == 0 ? 0 : 1;
    return failed;
}

static int adapters_command_rejects_extra_arguments(void) {
    const char *argv[] = {"swbt-daemon", "adapters", "--backend", "noop"};
    fake_adapter_inventory_t inventory = {0};
    const swbt_daemon_cli_ports_t ports = {
        .list_adapters = fake_list_adapters,
        .adapter_inventory_context = &inventory,
    };
    char err_buffer[2048];
    FILE *out = tmpfile();
    FILE *err = tmpfile();
    swbt_daemon_cli_dispatch_result_t result = {0};
    int failed = 0;

    if (out == NULL || err == NULL) {
        if (out != NULL) {
            (void)fclose(out);
        }
        if (err != NULL) {
            (void)fclose(err);
        }
        return 1;
    }

    result = swbt_daemon_cli_dispatch_with_ports(4, argv, out, err, &ports);
    failed += expect_true(result.handled, "handled");
    failed += expect_eq_int(result.exit_code, 1, "exit");
    failed += expect_eq_int(inventory.calls, 0, "inventory calls");
    failed += read_file(err, err_buffer, sizeof(err_buffer));
    failed += expect_contains(err_buffer, "adapters does not accept arguments", "error");

    failed += fclose(out) == 0 ? 0 : 1;
    failed += fclose(err) == 0 ? 0 : 1;
    return failed;
}

static int config_command_prints_effective_config(void) {
    const char *path = "daemon-cli-config.toml";
    const char *argv[] = {"swbt-daemon",
                          "config",
                          "--config",
                          path,
                          "--backend",
                          "noop",
                          "--link-key-db=tmp/link-keys.tlv"};
    const swbt_daemon_config_env_t env = {
        .report_period_us = "3333",
        .ipc_host = "127.0.0.2",
        .ipc_port = "37637",
    };
    const swbt_daemon_cli_ports_t ports = {
        .config_env = &env,
    };
    char out_buffer[4096];
    FILE *file = fopen(path, "wb");
    FILE *out = NULL;
    FILE *err = NULL;
    swbt_daemon_cli_dispatch_result_t result = {0};
    int failed = 0;

    if (file == NULL) {
        return 1;
    }
    if (fputs("[report]\n"
              "period_us = 16667\n"
              "[ipc]\n"
              "backlog = 4\n"
              "[active_reconnect]\n"
              "switch_address = \"01:23:45:67:89:ab\"\n",
              file) < 0 ||
        fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    out = tmpfile();
    err = tmpfile();
    if (out == NULL || err == NULL) {
        if (out != NULL) {
            (void)fclose(out);
        }
        if (err != NULL) {
            (void)fclose(err);
        }
        (void)remove(path);
        return 1;
    }

    result = swbt_daemon_cli_dispatch_with_ports(7, argv, out, err, &ports);
    failed += expect_true(result.handled, "handled");
    failed += expect_eq_int(result.exit_code, 0, "exit");
    failed += read_file(out, out_buffer, sizeof(out_buffer));
    failed += expect_contains(out_buffer, "backend=noop", "backend");
    failed += expect_contains(out_buffer, "config.path=daemon-cli-config.toml", "config path");
    failed += expect_contains(out_buffer, "link_key_db.path=tmp/link-keys.tlv", "link key db");
    failed += expect_contains(out_buffer, "runtime.report_period_us=3333", "env override");
    failed += expect_contains(out_buffer, "runtime.ipc.host=127.0.0.2", "ipc host");
    failed += expect_contains(out_buffer, "runtime.ipc.port=37637", "ipc port");
    failed += expect_contains(out_buffer, "runtime.ipc.backlog=4", "ipc backlog");
    failed +=
        expect_contains(out_buffer, "active_reconnect.effective_switch_address=01:23:45:67:89:AB",
                        "effective switch address");

    failed += fclose(out) == 0 ? 0 : 1;
    failed += fclose(err) == 0 ? 0 : 1;
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int config_command_reports_invalid_config_without_startup(void) {
    const char *path = "daemon-cli-invalid-config.toml";
    const char *argv[] = {"swbt-daemon", "config", "--config", path};
    char err_buffer[2048];
    FILE *file = fopen(path, "wb");
    FILE *out = NULL;
    FILE *err = NULL;
    swbt_daemon_cli_dispatch_result_t result = {0};
    int failed = 0;

    if (file == NULL) {
        return 1;
    }
    if (fputs("[report]\nperiod_us = 0\n", file) < 0 || fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    out = tmpfile();
    err = tmpfile();
    if (out == NULL || err == NULL) {
        if (out != NULL) {
            (void)fclose(out);
        }
        if (err != NULL) {
            (void)fclose(err);
        }
        (void)remove(path);
        return 1;
    }

    result = swbt_daemon_cli_dispatch(4, argv, out, err);
    failed += expect_true(result.handled, "handled");
    failed += expect_eq_int(result.exit_code, 1, "exit");
    failed += read_file(err, err_buffer, sizeof(err_buffer));
    failed += expect_contains(err_buffer, "config is invalid", "error");

    failed += fclose(out) == 0 ? 0 : 1;
    failed += fclose(err) == 0 ? 0 : 1;
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int no_command_is_not_handled_by_management_cli(void) {
    const char *argv[] = {"swbt-daemon", "--backend", "noop"};
    swbt_daemon_cli_dispatch_result_t result = swbt_daemon_cli_dispatch(3, argv, stdout, stderr);

    int failed = 0;
    failed += expect_eq_int(result.handled ? 1 : 0, 0, "handled");
    failed += expect_eq_int(result.exit_code, 0, "exit");
    return failed;
}

int main(void) {
    int failed = 0;
    failed += help_command_prints_management_surface();
    failed += adapters_command_lists_adapter_location_candidates();
    failed += adapters_command_fails_when_inventory_is_not_available();
    failed += adapters_command_rejects_extra_arguments();
    failed += config_command_prints_effective_config();
    failed += config_command_reports_invalid_config_without_startup();
    failed += no_command_is_not_handled_by_management_cli();
    return failed == 0 ? 0 : 1;
}
