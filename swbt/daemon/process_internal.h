#ifndef SWBT_DAEMON_PROCESS_INTERNAL_H
#define SWBT_DAEMON_PROCESS_INTERNAL_H

#include "daemon/process.h"

#include "domain/domain.h"
#include "btstack_bridge/output_report_handler.h"
#include "control/control.h"
#include "runtime/host.h"

struct swbt_daemon_process_backend {
    swbt_domain_daemon_backend_t daemon_backend;
    int (*ipc_start)(void *context, swbt_control_t *control);
    void (*ipc_stop)(void *context);
    const swbt_runtime_host_backend_t *runtime_backend;
};

struct swbt_daemon_process {
    swbt_daemon_config_t config;
    const swbt_daemon_process_backend_t *backend;
    void *backend_context;
    swbt_runtime_host_t runtime;
    bool initialized;
    bool running;
    bool ipc_started;
};

swbt_domain_t *swbt_daemon_process_app(swbt_daemon_process_t *host);

swbt_control_t *swbt_daemon_process_control(swbt_daemon_process_t *host);

swbt_btstack_output_report_handler_t *
swbt_daemon_process_output_handler(swbt_daemon_process_t *host);

#endif
