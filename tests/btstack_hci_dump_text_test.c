#include "btstack_bridge/hci_dump_text.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): test helper names both arguments.
static int expect_file_contains(const char *path, const char *needle) {
    FILE *file = fopen(path, "rb");
    char buffer[512];
    size_t read = 0;
    int result = 1;

    if (file == NULL) {
        return 1;
    }
    read = fread(buffer, 1, sizeof(buffer) - 1u, file);
    fclose(file);
    buffer[read] = '\0';
    if (strstr(buffer, needle) != NULL) {
        result = 0;
    }
    return result;
}

static int writes_hci_packet_as_text(void) {
    const char *path = "btstack-hci-dump-text-test.log";
    uint8_t packet[] = {0x01u, 0x02u, 0x03u};
    const hci_dump_t *dump = NULL;

    remove(path);

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_hci_dump_text_open(path), SWBT_BTSTACK_HCI_DUMP_TEXT_OK);
    dump = swbt_btstack_hci_dump_text_instance();
    failed += expect_true(dump != NULL);
    if (dump == NULL) {
        return 1;
    }
    failed += expect_true(dump->log_packet != NULL);
    if (dump->log_packet == NULL) {
        return 1;
    }
    dump->log_packet(0x01u, 0u, packet, sizeof(packet));
    swbt_btstack_hci_dump_text_close();

    failed += expect_file_contains(path, "swbt hci dump text v1");
    failed += expect_file_contains(path, "packet type=0x01 in=0 len=3 data=010203");
    remove(path);
    return failed;
}

static int rejects_missing_path(void) {
    int failed = 0;
    failed += expect_eq_int(swbt_btstack_hci_dump_text_open(NULL),
                            SWBT_BTSTACK_HCI_DUMP_TEXT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_hci_dump_text_open(""),
                            SWBT_BTSTACK_HCI_DUMP_TEXT_ERROR_INVALID_ARGUMENT);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += writes_hci_packet_as_text();
    failed += rejects_missing_path();
    return failed == 0 ? 0 : 1;
}
