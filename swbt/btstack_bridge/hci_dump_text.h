#ifndef SWBT_BTSTACK_BRIDGE_HCI_DUMP_TEXT_H
#define SWBT_BTSTACK_BRIDGE_HCI_DUMP_TEXT_H

#include "hci_dump.h"

typedef enum {
    SWBT_BTSTACK_HCI_DUMP_TEXT_OK = 0,
    SWBT_BTSTACK_HCI_DUMP_TEXT_ERROR_INVALID_ARGUMENT = -1,
    SWBT_BTSTACK_HCI_DUMP_TEXT_ERROR_OPEN_FAILED = -2,
} swbt_btstack_hci_dump_text_result_t;

swbt_btstack_hci_dump_text_result_t swbt_btstack_hci_dump_text_open(const char *path);

void swbt_btstack_hci_dump_text_close(void);

const hci_dump_t *swbt_btstack_hci_dump_text_instance(void);

#endif
