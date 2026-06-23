// NOLINTNEXTLINE(bugprone-reserved-identifier): POSIX feature test macro.
#define _POSIX_C_SOURCE 200112L

#include "btstack_bridge/production_btstack.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <stdlib.h>
#endif

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
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

    if (adapter == NULL || adapter->platform_start == NULL) {
        return 1;
    }
    if (set_hci_dump_env("btstack-hci-dump-missing-dir/hci.log") != 0) {
        return 1;
    }
    failed += expect_eq_int(adapter->platform_start(NULL), -1);
    clear_hci_dump_env();
    return failed;
}

int main(void) {
    int failed = 0;
    failed += missing_hci_dump_path_is_noop();
    failed += explicit_hci_dump_open_failure_rejects_platform_start();
    return failed == 0 ? 0 : 1;
}
