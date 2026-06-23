#ifndef SWBT_BTSTACK_BRIDGE_PRODUCTION_BTSTACK_H
#define SWBT_BTSTACK_BRIDGE_PRODUCTION_BTSTACK_H

#include "btstack_bridge/production_adapter.h"

int swbt_btstack_production_hci_dump_start(const char *path);

const swbt_btstack_production_adapter_t *swbt_btstack_production_adapter(void);

#endif
