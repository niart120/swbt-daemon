#ifndef SWBT_DAEMON_RUNTIME_H
#define SWBT_DAEMON_RUNTIME_H

#include <stdbool.h>

#include "btstack_bridge/output_report_handler.h"
#include "core/state_mailbox.h"
#include "daemon/config.h"
#include "ipc/ipc_session.h"
#include "switch/switch_controller_state.h"

typedef enum {
    SWBT_DAEMON_RUNTIME_OK = 0,
    SWBT_DAEMON_RUNTIME_ERROR_INVALID_ARGUMENT = -1,
    SWBT_DAEMON_RUNTIME_ERROR_BACKEND = -2,
} swbt_daemon_runtime_result_t;

typedef swbt_state_t (*swbt_daemon_state_provider_t)(void *context);

typedef struct {
    int (*ipc_start)(void *context, swbt_ipc_session_t *session);
    void (*ipc_stop)(void *context);
    int (*hid_register)(void *context);
    void (*hid_stop)(void *context);
    void (*output_handler_start)(void *context, swbt_btstack_output_report_handler_t *handler);
    void (*output_handler_stop)(void *context);
    int (*report_timer_start)(void *context, swbt_daemon_state_provider_t state_provider,
                              void *state_context);
    void (*report_timer_stop)(void *context);
} swbt_daemon_runtime_backend_t;

typedef struct {
    swbt_daemon_config_t config;
    const swbt_daemon_runtime_backend_t *backend;
    void *backend_context;
    swbt_state_mailbox_t mailbox;
    swbt_ipc_session_t ipc_session;
    swbt_btstack_output_report_handler_t output_handler;
    bool initialized;
    bool running;
    bool ipc_started;
    bool hid_registered;
    bool output_handler_started;
    bool report_timer_started;
} swbt_daemon_runtime_t;

swbt_daemon_runtime_result_t swbt_daemon_runtime_init(swbt_daemon_runtime_t *runtime,
                                                      const swbt_daemon_config_t *config,
                                                      const swbt_daemon_runtime_backend_t *backend,
                                                      void *backend_context);

swbt_daemon_runtime_result_t swbt_daemon_runtime_start(swbt_daemon_runtime_t *runtime);

void swbt_daemon_runtime_stop(swbt_daemon_runtime_t *runtime);

bool swbt_daemon_runtime_is_running(const swbt_daemon_runtime_t *runtime);

swbt_ipc_session_t *swbt_daemon_runtime_ipc_session(swbt_daemon_runtime_t *runtime);

swbt_state_mailbox_t *swbt_daemon_runtime_mailbox(swbt_daemon_runtime_t *runtime);

swbt_btstack_output_report_handler_t *
swbt_daemon_runtime_output_handler(swbt_daemon_runtime_t *runtime);

const swbt_daemon_runtime_backend_t *swbt_daemon_runtime_noop_backend(void);

int swbt_daemon_main_with_backend(const swbt_daemon_config_t *config,
                                  const swbt_daemon_runtime_backend_t *backend,
                                  void *backend_context);

#endif
