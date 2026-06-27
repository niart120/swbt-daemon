#ifndef SWBT_BTSTACK_BRIDGE_INPUT_REPORT_TIMER_ADAPTER_H
#define SWBT_BTSTACK_BRIDGE_INPUT_REPORT_TIMER_ADAPTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/input_report_scheduler.h"
#include "btstack_bridge/subcommand_reply_queue.h"
#include "btstack_run_loop.h"

typedef enum {
    SWBT_BTSTACK_INPUT_REPORT_TIMER_OK = 0,
    SWBT_BTSTACK_INPUT_REPORT_TIMER_NOT_DUE = 1,
    SWBT_BTSTACK_INPUT_REPORT_TIMER_STOPPED = 2,
    SWBT_BTSTACK_INPUT_REPORT_TIMER_PENDING = 3,
    SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT = -1,
    SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_SCHEDULER = -2,
    SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_REPLY_QUEUE = -3,
} swbt_btstack_input_report_timer_result_t;

typedef swbt_state_t (*swbt_btstack_input_report_timer_state_provider_t)(void *context);

typedef enum {
    SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_OK = 0,
    SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_FAILED = 1,
} swbt_btstack_input_report_timer_report_send_result_t;

typedef void (*swbt_btstack_input_report_timer_report_tick_observer_t)(
    void *context, uint64_t now_us,
    swbt_btstack_input_report_timer_report_send_result_t send_result);

typedef int (*swbt_btstack_input_report_timer_hid_sender_t)(void *context, uint16_t hid_cid,
                                                            const uint8_t *message,
                                                            size_t message_size);

typedef struct {
    void (*set_timer_handler)(btstack_timer_source_t *timer,
                              void (*process)(btstack_timer_source_t *timer));
    void (*set_timer_context)(btstack_timer_source_t *timer, void *context);
    void (*set_timer)(btstack_timer_source_t *timer, uint32_t timeout_ms);
    void (*add_timer)(btstack_timer_source_t *timer);
    int (*remove_timer)(btstack_timer_source_t *timer);
    uint32_t (*get_time_ms)(void);
    void (*request_can_send_now_event)(uint16_t hid_cid);
    int (*send_interrupt_message)(uint16_t hid_cid, const uint8_t *message, uint16_t message_len);
} swbt_btstack_input_report_timer_backend_t;

typedef struct {
    const swbt_btstack_input_report_timer_backend_t *backend;
    swbt_btstack_input_report_timer_hid_sender_t hid_sender;
    void *hid_sender_context;
    swbt_btstack_input_report_timer_state_provider_t state_provider;
    void *state_context;
    swbt_btstack_input_report_timer_report_tick_observer_t report_tick_observer;
    void *report_tick_context;
    swbt_btstack_input_report_scheduler_config_t scheduler_config;
} swbt_btstack_input_report_timer_adapter_config_t;

typedef struct {
    uint16_t hid_cid;
    uint64_t now_us;
} swbt_btstack_input_report_timer_start_options_t;

typedef struct {
    const swbt_btstack_input_report_timer_backend_t *backend;
    swbt_btstack_input_report_timer_hid_sender_t hid_sender;
    void *hid_sender_context;
    swbt_btstack_input_report_timer_state_provider_t state_provider;
    void *state_context;
    swbt_btstack_input_report_timer_report_tick_observer_t report_tick_observer;
    void *report_tick_context;
    swbt_btstack_input_report_scheduler_t scheduler;
    swbt_btstack_subcommand_reply_queue_t reply_queue;
    btstack_timer_source_t timer;
    uint16_t hid_cid;
    bool initialized;
    bool running;
    bool timer_pending;
    bool can_send_pending;
    bool neutral_send_pending;
    uint64_t periodic_holdoff_until_us;
} swbt_btstack_input_report_timer_adapter_t;

const swbt_btstack_input_report_timer_backend_t *
swbt_btstack_input_report_timer_backend_btstack(void);

swbt_btstack_input_report_timer_result_t swbt_btstack_input_report_timer_adapter_init(
    swbt_btstack_input_report_timer_adapter_t *adapter,
    const swbt_btstack_input_report_timer_adapter_config_t *config);

swbt_btstack_input_report_timer_result_t swbt_btstack_input_report_timer_adapter_start(
    swbt_btstack_input_report_timer_adapter_t *adapter,
    swbt_btstack_input_report_timer_start_options_t options);

swbt_btstack_input_report_timer_result_t swbt_btstack_input_report_timer_adapter_on_timer(
    swbt_btstack_input_report_timer_adapter_t *adapter);

swbt_btstack_input_report_timer_result_t swbt_btstack_input_report_timer_adapter_on_can_send_now(
    swbt_btstack_input_report_timer_adapter_t *adapter);

swbt_btstack_input_report_timer_result_t
swbt_btstack_input_report_timer_adapter_enqueue_subcommand_reply(
    swbt_btstack_input_report_timer_adapter_t *adapter, uint16_t hid_cid, const uint8_t *report,
    size_t report_size);

swbt_btstack_input_report_timer_result_t swbt_btstack_input_report_timer_adapter_send_neutral_now(
    swbt_btstack_input_report_timer_adapter_t *adapter);

void swbt_btstack_input_report_timer_adapter_stop(
    swbt_btstack_input_report_timer_adapter_t *adapter);

#endif
