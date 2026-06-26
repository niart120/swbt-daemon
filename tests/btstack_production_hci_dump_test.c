// NOLINTNEXTLINE(bugprone-reserved-identifier): POSIX feature test macro.
#define _POSIX_C_SOURCE 200112L

#include "btstack_bridge/production_btstack.h"

#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <stdlib.h>
#endif

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int file_exists(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }
    fclose(file);
    return 1;
}

static int set_hci_dump_env(const char *path) {
#if defined(_WIN32)
    return SetEnvironmentVariableA("SWBT_HCI_DUMP_TRACE_PATH", path) ? 0 : 1;
#else
    return setenv("SWBT_HCI_DUMP_TRACE_PATH", path, 1);
#endif
}

static void clear_hci_dump_env(void) {
#if defined(_WIN32)
    (void)SetEnvironmentVariableA("SWBT_HCI_DUMP_TRACE_PATH", NULL);
#else
    unsetenv("SWBT_HCI_DUMP_TRACE_PATH");
#endif
}

static int missing_hci_dump_path_is_noop(void) {
    int failed = 0;
    failed += expect_eq_int(swbt_btstack_production_hci_dump_start(NULL), 0);
    failed += expect_eq_int(swbt_btstack_production_hci_dump_start(""), 0);
    return failed;
}

static int explicit_hci_dump_open_failure_rejects_platform_start(void) {
    int failed = 0;
    const swbt_btstack_production_adapter_t *adapter = swbt_btstack_production_adapter();

    if (adapter == NULL || adapter->platform.start == NULL) {
        return 1;
    }
    if (set_hci_dump_env("btstack-hci-dump-missing-dir/hci.log") != 0) {
        return 1;
    }
    failed += expect_eq_int(adapter->platform.start(NULL), -1);
    clear_hci_dump_env();
    return failed;
}

static int experimental_link_key_db_path_creates_tlv_file_on_platform_start(void) {
    int failed = 0;
    const char *path = "btstack-experimental-link-key-db-test.tlv";
    const swbt_btstack_production_adapter_t *adapter = swbt_btstack_production_adapter();

    if (adapter == NULL || adapter->platform.start == NULL || adapter->platform.stop == NULL) {
        return 1;
    }

    clear_hci_dump_env();
    (void)remove(path);
    failed += expect_eq_int(swbt_btstack_production_experimental_link_key_db_configure(""), -1);
    failed += expect_eq_int(swbt_btstack_production_experimental_link_key_db_configure(path), 0);
    failed += expect_eq_int(adapter->platform.start(NULL), 0);
    failed += expect_eq_int(file_exists(path), 1);
    adapter->platform.stop(NULL);
    failed += expect_eq_int(swbt_btstack_production_experimental_link_key_db_configure(NULL), 0);
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

int main(void) {
    int failed = 0;
    failed += missing_hci_dump_path_is_noop();
    failed += explicit_hci_dump_open_failure_rejects_platform_start();
    failed += experimental_link_key_db_path_creates_tlv_file_on_platform_start();
    return failed == 0 ? 0 : 1;
}
