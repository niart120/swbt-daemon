// NOLINTNEXTLINE(bugprone-reserved-identifier): POSIX feature test macro.
#define _POSIX_C_SOURCE 200112L

#include "btstack_bridge/production_btstack_impl.h"

#include "gap.h"
#include "hci.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <stdlib.h>
#endif

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_bytes_eq(const uint8_t *actual, const uint8_t *expected, size_t size) {
    return memcmp(actual, expected, size) == 0 ? 0 : 1;
}

static int file_exists(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }
    fclose(file);
    return 1;
}

static long file_size(const char *path) {
    long size = -1;
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return -1;
    }
    if (fseek(file, 0L, SEEK_END) == 0) {
        size = ftell(file);
    }
    fclose(file);
    return size;
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
    const swbt_btstack_production_ports_t *adapter = swbt_btstack_production_ports_btstack();

    if (adapter == NULL || adapter->device.platform_start == NULL) {
        return 1;
    }
    failed += expect_eq_int(
        swbt_btstack_production_hci_dump_configure("btstack-hci-dump-missing-dir/hci.log"), 0);
    failed += expect_eq_int(adapter->device.platform_start(NULL), -1);
    failed += expect_eq_int(swbt_btstack_production_hci_dump_configure(NULL), 0);
    return failed;
}

static int hci_dump_env_path_is_ignored_without_cli_configuration(void) {
    int failed = 0;
    const swbt_btstack_production_ports_t *adapter = swbt_btstack_production_ports_btstack();

    if (adapter == NULL || adapter->device.platform_start == NULL ||
        adapter->device.platform_stop == NULL) {
        return 1;
    }
    if (set_hci_dump_env("btstack-hci-dump-missing-dir/hci.log") != 0) {
        return 1;
    }
    failed += expect_eq_int(swbt_btstack_production_hci_dump_configure(NULL), 0);
    failed += expect_eq_int(adapter->device.platform_start(NULL), 0);
    adapter->device.platform_stop(NULL);
    clear_hci_dump_env();
    return failed;
}

static int link_key_db_path_creates_tlv_file_on_platform_start(void) {
    int failed = 0;
    const char *path = "btstack-link-key-db-test.tlv";
    const swbt_btstack_production_ports_t *adapter = swbt_btstack_production_ports_btstack();

    if (adapter == NULL || adapter->device.platform_start == NULL ||
        adapter->device.platform_stop == NULL) {
        return 1;
    }

    clear_hci_dump_env();
    (void)remove(path);
    failed += expect_eq_int(swbt_btstack_production_link_key_db_configure(""), -1);
    failed += expect_eq_int(swbt_btstack_production_link_key_db_configure(path), 0);
    failed += expect_eq_int(adapter->device.platform_start(NULL), 0);
    failed += expect_eq_int(file_exists(path), 1);
    adapter->device.platform_stop(NULL);
    failed += expect_eq_int(swbt_btstack_production_link_key_db_configure(NULL), 0);
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int link_key_db_stores_link_key_notification(void) {
    int failed = 0;
    const char *path = "btstack-link-key-db-notification-test.tlv";
    const swbt_btstack_production_ports_t *adapter = swbt_btstack_production_ports_btstack();
    uint8_t link_key_notification[] = {
        HCI_EVENT_LINK_KEY_NOTIFICATION,
        23,
        0x06,
        0x05,
        0x04,
        0x03,
        0x02,
        0x01,
        0x10,
        0x11,
        0x12,
        0x13,
        0x14,
        0x15,
        0x16,
        0x17,
        0x18,
        0x19,
        0x1a,
        0x1b,
        0x1c,
        0x1d,
        0x1e,
        0x1f,
        UNAUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192,
    };

    if (adapter == NULL || adapter->device.platform_start == NULL ||
        adapter->device.platform_stop == NULL) {
        return 1;
    }

    clear_hci_dump_env();
    (void)remove(path);
    failed += expect_eq_int(swbt_btstack_production_link_key_db_configure(path), 0);
    failed += expect_eq_int(adapter->device.platform_start(NULL), 0);
    hci_emit_btstack_event(link_key_notification, sizeof(link_key_notification), 0);
    adapter->device.platform_stop(NULL);
    failed += file_size(path) > 8L ? 0 : 1;
    failed += expect_eq_int(swbt_btstack_production_link_key_db_configure(NULL), 0);
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int link_key_db_refreshes_existing_link_key_notification(void) {
    int failed = 0;
    const char *path = "btstack-link-key-db-refresh-test.tlv";
    const swbt_btstack_production_ports_t *adapter = swbt_btstack_production_ports_btstack();
    bd_addr_t address = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    const uint8_t expected_link_key[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
                                         0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f};
    link_key_t stored_link_key = {0};
    link_key_type_t stored_link_key_type = INVALID_LINK_KEY;
    uint8_t first_link_key_notification[] = {
        HCI_EVENT_LINK_KEY_NOTIFICATION,
        23,
        0x06,
        0x05,
        0x04,
        0x03,
        0x02,
        0x01,
        0x10,
        0x11,
        0x12,
        0x13,
        0x14,
        0x15,
        0x16,
        0x17,
        0x18,
        0x19,
        0x1a,
        0x1b,
        0x1c,
        0x1d,
        0x1e,
        0x1f,
        UNAUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192,
    };
    uint8_t refreshed_link_key_notification[] = {
        HCI_EVENT_LINK_KEY_NOTIFICATION,
        23,
        0x06,
        0x05,
        0x04,
        0x03,
        0x02,
        0x01,
        0x20,
        0x21,
        0x22,
        0x23,
        0x24,
        0x25,
        0x26,
        0x27,
        0x28,
        0x29,
        0x2a,
        0x2b,
        0x2c,
        0x2d,
        0x2e,
        0x2f,
        AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192,
    };

    if (adapter == NULL || adapter->device.platform_start == NULL ||
        adapter->device.platform_stop == NULL) {
        return 1;
    }

    clear_hci_dump_env();
    (void)remove(path);
    failed += expect_eq_int(swbt_btstack_production_link_key_db_configure(path), 0);
    failed += expect_eq_int(adapter->device.platform_start(NULL), 0);
    hci_emit_btstack_event(first_link_key_notification, sizeof(first_link_key_notification), 0);
    hci_emit_btstack_event(refreshed_link_key_notification, sizeof(refreshed_link_key_notification),
                           0);
    failed += expect_eq_int(
        gap_get_link_key_for_bd_addr(address, stored_link_key, &stored_link_key_type) ? 1 : 0, 1);
    failed += expect_bytes_eq(stored_link_key, expected_link_key, sizeof(expected_link_key));
    failed +=
        expect_eq_int(stored_link_key_type, AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192);
    adapter->device.platform_stop(NULL);
    failed += expect_eq_int(swbt_btstack_production_link_key_db_configure(NULL), 0);
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

int main(void) {
    int failed = 0;
    failed += missing_hci_dump_path_is_noop();
    failed += explicit_hci_dump_open_failure_rejects_platform_start();
    failed += hci_dump_env_path_is_ignored_without_cli_configuration();
    failed += link_key_db_path_creates_tlv_file_on_platform_start();
    failed += link_key_db_stores_link_key_notification();
    failed += link_key_db_refreshes_existing_link_key_notification();
    return failed == 0 ? 0 : 1;
}
