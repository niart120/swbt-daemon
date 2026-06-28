#ifndef SWBT_DAEMON_BTSTACK_PROCESS_BACKEND_H
#define SWBT_DAEMON_BTSTACK_PROCESS_BACKEND_H

#include "daemon/process.h"
#include "runtime/host.h"

const swbt_runtime_host_backend_t *swbt_daemon_btstack_runtime_backend(void);

const swbt_daemon_process_backend_t *swbt_daemon_btstack_process_backend(void);

#endif
