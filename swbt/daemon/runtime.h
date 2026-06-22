#ifndef SWBT_DAEMON_RUNTIME_H
#define SWBT_DAEMON_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/output_report_handler.h"
#include "core/state_mailbox.h"
#include "daemon/config.h"
#include "ipc/ipc_session.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_device_info.h"
#include "switch/switch_player_lights.h"
#include "switch/switch_spi.h"

typedef enum {
    SWBT_DAEMON_RUNTIME_OK = 0,
    SWBT_DAEMON_RUNTIME_ERROR_INVALID_ARGUMENT = -1,
    SWBT_DAEMON_RUNTIME_ERROR_BACKEND = -2,
} swbt_daemon_runtime_result_t;

typedef swbt_state_t (*swbt_daemon_state_provider_t)(void *context);

typedef struct {
    swbt_ipc_daemon_backend_t daemon_backend;
    int (*ipc_start)(void *context, swbt_ipc_session_t *session);
    void (*ipc_stop)(void *context);
    int (*hid_register)(void *context);
    void (*hid_stop)(void *context);
    void (*output_handler_start)(void *context, swbt_btstack_output_report_handler_t *handler);
    void (*output_handler_stop)(void *context);
    int (*report_timer_start)(void *context, swbt_daemon_state_provider_t state_provider,
                              void *state_context);
    void (*report_timer_stop)(void *context);
    int (*report_timer_send_neutral_now)(void *context);
    int (*subcommand_reply_enqueue)(void *context, uint16_t hid_cid, const uint8_t *report,
                                    size_t report_size);
    int (*read_device_info)(void *context, swbt_switch_device_info_t *out_device_info);
    uint32_t (*time_ms)(void *context);
} swbt_daemon_runtime_backend_t;

typedef struct {
    swbt_daemon_config_t config;
    const swbt_daemon_runtime_backend_t *backend;
    void *backend_context;
    swbt_state_mailbox_t mailbox;
    swbt_ipc_session_t ipc_session;
    swbt_btstack_output_report_handler_t output_handler;
    swbt_switch_spi_t spi;
    swbt_switch_player_lights_state_t player_lights;
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

swbt_daemon_runtime_result_t swbt_daemon_runtime_send_neutral_now(swbt_daemon_runtime_t *runtime);

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
