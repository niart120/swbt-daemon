#ifndef SWBT_BTSTACK_BRIDGE_PRODUCTION_BTSTACK_IMPL_H
#define SWBT_BTSTACK_BRIDGE_PRODUCTION_BTSTACK_IMPL_H

#include <stdio.h>

#include "btstack_bridge/production_ports.h"

int swbt_btstack_production_hci_dump_start(const char *path);

int swbt_btstack_production_hci_dump_configure(const char *path);

int swbt_btstack_production_link_key_db_configure(const char *path);

int swbt_btstack_production_impl_configure_adapter_location(const char *location);

int swbt_btstack_production_list_adapter_locations(void *context, FILE *out, FILE *err);

const swbt_btstack_production_ports_t *swbt_btstack_production_ports_btstack(void);

#endif
