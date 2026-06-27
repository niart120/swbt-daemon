#ifndef SWBT_BTSTACK_BRIDGE_PRODUCTION_BTSTACK_H
#define SWBT_BTSTACK_BRIDGE_PRODUCTION_BTSTACK_H

#include <stdio.h>

#include "btstack_bridge/production_adapter.h"

int swbt_btstack_production_hci_dump_start(const char *path);

int swbt_btstack_production_hci_dump_configure(const char *path);

int swbt_btstack_production_link_key_db_configure(const char *path);

int swbt_btstack_production_adapter_location_configure(const char *location);

int swbt_btstack_production_list_adapter_locations(void *context, FILE *out, FILE *err);

const swbt_btstack_production_adapter_t *swbt_btstack_production_adapter(void);

#endif
