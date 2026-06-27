#ifndef SWBT_RUNTIME_HOST_H
#define SWBT_RUNTIME_HOST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "domain/domain.h"
#include "btstack_bridge/output_report_handler.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_device_info.h"

typedef enum {
    SWBT_RUNTIME_HOST_OK = 0,
    SWBT_RUNTIME_HOST_PENDING = 1,
    SWBT_RUNTIME_HOST_ERROR_INVALID_ARGUMENT = -1,
    SWBT_RUNTIME_HOST_ERROR_BACKEND = -2,
} swbt_runtime_host_result_t;

typedef swbt_state_t (*swbt_runtime_state_provider_t)(void *context);

typedef struct {
    int (*hid_register)(void *context);
    void (*hid_stop)(void *context);
    void (*output_handler_start)(void *context, swbt_btstack_output_report_handler_t *handler);
    void (*output_handler_stop)(void *context);
    int (*report_timer_start)(void *context, swbt_runtime_state_provider_t state_provider,
                              void *state_context);
    void (*report_timer_stop)(void *context);
    int (*report_timer_send_neutral_now)(void *context);
    int (*subcommand_reply_enqueue)(void *context, uint16_t hid_cid, const uint8_t *report,
                                    size_t report_size);
    int (*read_device_info)(void *context, swbt_switch_device_info_t *out_device_info);
    uint32_t (*time_ms)(void *context);
} swbt_runtime_host_backend_t;

typedef struct {
    swbt_domain_t *app;
    swbt_switch_report_options_t report_options;
    swbt_switch_device_info_t device_info;
} swbt_runtime_host_config_t;

typedef struct {
    bool initialized;
    bool running;
    bool hid_registered;
    bool output_handler_started;
    bool report_timer_started;
} swbt_runtime_host_status_t;

typedef struct {
    const swbt_runtime_host_backend_t *backend;
    void *backend_context;
    swbt_domain_t *app;
    swbt_switch_report_options_t report_options;
    swbt_switch_device_info_t device_info;
    swbt_btstack_output_report_handler_t output_handler;
    bool initialized;
    bool running;
    bool hid_registered;
    bool output_handler_started;
    bool report_timer_started;
} swbt_runtime_host_t;

swbt_runtime_host_result_t swbt_runtime_host_init(swbt_runtime_host_t *runtime,
                                                  const swbt_runtime_host_config_t *config,
                                                  const swbt_runtime_host_backend_t *backend,
                                                  void *backend_context);

swbt_runtime_host_result_t swbt_runtime_host_start(swbt_runtime_host_t *runtime);

swbt_runtime_host_result_t swbt_runtime_host_send_neutral_now(swbt_runtime_host_t *runtime);

void swbt_runtime_host_stop(swbt_runtime_host_t *runtime);

bool swbt_runtime_host_is_running(const swbt_runtime_host_t *runtime);

swbt_runtime_host_result_t swbt_runtime_host_status(const swbt_runtime_host_t *runtime,
                                                    swbt_runtime_host_status_t *out_status);

swbt_btstack_output_report_handler_t *
swbt_runtime_host_output_handler(swbt_runtime_host_t *runtime);

#endif
