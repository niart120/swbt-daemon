#ifndef SWBT_BTSTACK_BRIDGE_PRODUCTION_BTSTACK_H
#define SWBT_BTSTACK_BRIDGE_PRODUCTION_BTSTACK_H

#include "daemon/production_backend_ops.h"

int swbt_btstack_production_hci_dump_start(const char *path);

const swbt_daemon_production_backend_ops_t *swbt_btstack_production_backend_ops(void);

#endif
