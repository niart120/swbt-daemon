#ifndef SWBT_DAEMON_HOST_H
#define SWBT_DAEMON_HOST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "application/app.h"
#include "btstack_bridge/output_report_handler.h"
#include "daemon/config.h"
#include "runtime/host.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_device_info.h"

typedef enum {
    SWBT_DAEMON_HOST_OK = 0,
    SWBT_DAEMON_HOST_PENDING = 1,
    SWBT_DAEMON_HOST_ERROR_INVALID_ARGUMENT = -1,
    SWBT_DAEMON_HOST_ERROR_BACKEND = -2,
} swbt_daemon_host_result_t;

typedef swbt_state_t (*swbt_daemon_host_state_provider_t)(void *context);

typedef struct {
    swbt_app_daemon_backend_t daemon_backend;
    int (*ipc_start)(void *context, swbt_app_t *app);
    void (*ipc_stop)(void *context);
    int (*hid_register)(void *context);
    void (*hid_stop)(void *context);
    void (*output_handler_start)(void *context, swbt_btstack_output_report_handler_t *handler);
    void (*output_handler_stop)(void *context);
    int (*report_timer_start)(void *context, swbt_daemon_host_state_provider_t state_provider,
                              void *state_context);
    void (*report_timer_stop)(void *context);
    int (*report_timer_send_neutral_now)(void *context);
    int (*subcommand_reply_enqueue)(void *context, uint16_t hid_cid, const uint8_t *report,
                                    size_t report_size);
    int (*read_device_info)(void *context, swbt_switch_device_info_t *out_device_info);
    uint32_t (*time_ms)(void *context);
} swbt_daemon_host_backend_t;

typedef struct {
    swbt_daemon_config_t config;
    const swbt_daemon_host_backend_t *backend;
    void *backend_context;
    swbt_app_t *app;
    swbt_runtime_host_t runtime;
    swbt_runtime_host_backend_t runtime_backend;
    bool initialized;
    bool running;
    bool ipc_started;
} swbt_daemon_host_t;

swbt_daemon_host_result_t swbt_daemon_host_init(swbt_daemon_host_t *host,
                                                const swbt_daemon_config_t *config,
                                                const swbt_daemon_host_backend_t *backend,
                                                void *backend_context);

swbt_daemon_host_result_t swbt_daemon_host_start(swbt_daemon_host_t *host);

swbt_daemon_host_result_t swbt_daemon_host_send_neutral_now(swbt_daemon_host_t *host);

void swbt_daemon_host_stop(swbt_daemon_host_t *host);
void swbt_daemon_host_destroy(swbt_daemon_host_t *host);

bool swbt_daemon_host_is_running(const swbt_daemon_host_t *host);

swbt_app_t *swbt_daemon_host_app(swbt_daemon_host_t *host);

swbt_btstack_output_report_handler_t *swbt_daemon_host_output_handler(swbt_daemon_host_t *host);

const swbt_daemon_host_backend_t *swbt_daemon_host_noop_backend(void);

int swbt_daemon_main_with_host_backend(const swbt_daemon_config_t *config,
                                       const swbt_daemon_host_backend_t *backend,
                                       void *backend_context);

#endif
