#include "btstack_bridge/input_report_timer_adapter.h"

#include "btstack_bridge/hid_port.h"
#include "btstack_bridge/timer_port.h"

#define SWBT_BTSTACK_HIDP_INPUT_REPORT_HEADER 0xA1u
#define SWBT_BTSTACK_HIDP_MAX_INPUT_MESSAGE_SIZE (1u + SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE)
#define SWBT_BTSTACK_REPLY_PERIODIC_HOLDOFF_US 300000u

static uint32_t delay_ms_until(uint64_t now_us, uint64_t deadline_us) {
    if (deadline_us <= now_us) {
        return 0u;
    }

    const uint64_t delta_us = deadline_us - now_us;
    const uint64_t delay_ms = (delta_us + 999u) / 1000u;
    return delay_ms > UINT32_MAX ? UINT32_MAX : (uint32_t)delay_ms;
}

static swbt_btstack_input_report_timer_result_t
map_scheduler_result(swbt_btstack_input_report_result_t result) {
    switch (result) {
    case SWBT_BTSTACK_INPUT_REPORT_OK:
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_OK;
    case SWBT_BTSTACK_INPUT_REPORT_NOT_DUE:
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_NOT_DUE;
    case SWBT_BTSTACK_INPUT_REPORT_STOPPED:
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_STOPPED;
    case SWBT_BTSTACK_INPUT_REPORT_ERROR_INVALID_ARGUMENT:
    case SWBT_BTSTACK_INPUT_REPORT_ERROR_BUILD_FAILED:
    case SWBT_BTSTACK_INPUT_REPORT_ERROR_SEND_FAILED:
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_SCHEDULER;
    }
    return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_SCHEDULER;
}

static swbt_btstack_input_report_timer_result_t
map_reply_queue_result(swbt_btstack_subcommand_reply_queue_result_t result) {
    switch (result) {
    case SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK:
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_OK;
    case SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_EMPTY:
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_NOT_DUE;
    case SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_ARGUMENT:
    case SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_FULL:
    case SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_REPORT_SIZE:
    case SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_SEND_FAILED:
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_REPLY_QUEUE;
    }
    return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_REPLY_QUEUE;
}

static bool backend_is_valid(const swbt_btstack_input_report_timer_backend_t *backend) {
    return backend != NULL && backend->set_timer_handler != NULL &&
           backend->set_timer_context != NULL && backend->set_timer != NULL &&
           backend->add_timer != NULL && backend->remove_timer != NULL &&
           backend->get_time_ms != NULL && backend->request_can_send_now_event != NULL &&
           backend->send_interrupt_message != NULL;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static void notify_report_tick(swbt_btstack_input_report_timer_adapter_t *adapter, uint64_t now_us,
                               swbt_btstack_input_report_result_t tick_result) {
    swbt_btstack_input_report_timer_report_send_result_t send_result;

    if (adapter == NULL || adapter->report_tick_observer == NULL) {
        return;
    }
    if (tick_result == SWBT_BTSTACK_INPUT_REPORT_OK) {
        send_result = SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_OK;
    } else if (tick_result == SWBT_BTSTACK_INPUT_REPORT_ERROR_SEND_FAILED) {
        send_result = SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_FAILED;
    } else {
        return;
    }

    adapter->report_tick_observer(adapter->report_tick_context, now_us, send_result);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

static swbt_btstack_input_report_timer_result_t
schedule_next_timer(swbt_btstack_input_report_timer_adapter_t *adapter, uint64_t now_us) {
    const uint64_t next_deadline_us =
        swbt_btstack_input_report_scheduler_next_deadline_us(&adapter->scheduler);
    const uint64_t scheduled_deadline_us = adapter->periodic_holdoff_until_us > next_deadline_us
                                               ? adapter->periodic_holdoff_until_us
                                               : next_deadline_us;
    const uint32_t delay_ms = delay_ms_until(now_us, scheduled_deadline_us);
    adapter->backend->set_timer(&adapter->timer, delay_ms);
    adapter->backend->add_timer(&adapter->timer);
    adapter->timer_pending = true;
    return SWBT_BTSTACK_INPUT_REPORT_TIMER_OK;
}

static swbt_btstack_input_report_timer_result_t
holdoff_periodic_after_reply(swbt_btstack_input_report_timer_adapter_t *adapter, uint64_t now_us) {
    adapter->periodic_holdoff_until_us = now_us + SWBT_BTSTACK_REPLY_PERIODIC_HOLDOFF_US;
    adapter->can_send_pending = false;
    if (adapter->timer_pending) {
        (void)adapter->backend->remove_timer(&adapter->timer);
        adapter->timer_pending = false;
    }

    return schedule_next_timer(adapter, now_us);
}

static void cancel_pending_timer(swbt_btstack_input_report_timer_adapter_t *adapter) {
    if (adapter->timer_pending) {
        (void)adapter->backend->remove_timer(&adapter->timer);
        adapter->timer_pending = false;
    }
}

static int send_hidp_input_report(swbt_btstack_input_report_timer_adapter_t *adapter,
                                  uint16_t hid_cid, const uint8_t *report, size_t report_size) {
    if (adapter == NULL || report == NULL || report_size == 0u ||
        report_size > (SWBT_BTSTACK_HIDP_MAX_INPUT_MESSAGE_SIZE - 1u)) {
        return -1;
    }

    uint8_t message[SWBT_BTSTACK_HIDP_MAX_INPUT_MESSAGE_SIZE] = {0};
    message[0] = SWBT_BTSTACK_HIDP_INPUT_REPORT_HEADER;
    for (size_t index = 0; index < report_size; ++index) {
        message[index + 1u] = report[index];
    }

    const bool uses_shared_timer =
        report_size > 1u && report[0] == SWBT_SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY;
    if (uses_shared_timer) {
        message[2] = adapter->scheduler.timer;
    }

    const int result =
        adapter->backend->send_interrupt_message(hid_cid, message, (uint16_t)(report_size + 1u));
    if (result == 0 && uses_shared_timer) {
        adapter->scheduler.timer = (uint8_t)(adapter->scheduler.timer + 1u);
    }
    return result;
}

static swbt_btstack_input_report_timer_result_t
send_neutral_report(swbt_btstack_input_report_timer_adapter_t *adapter) {
    swbt_switch_report_options_t report_options = adapter->scheduler.report_options;
    report_options.timer = adapter->scheduler.timer;
    const swbt_state_t state = swbt_state_neutral();
    size_t written = 0u;

    if (swbt_switch_build_standard_full_report(&state, &report_options, adapter->scheduler.scratch,
                                               sizeof(adapter->scheduler.scratch),
                                               &written) != SWBT_SWITCH_REPORT_OK) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_SCHEDULER;
    }
    if (send_hidp_input_report(adapter, adapter->hid_cid, adapter->scheduler.scratch, written) !=
        0) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_SCHEDULER;
    }

    adapter->scheduler.timer = (uint8_t)(adapter->scheduler.timer + 1u);
    return SWBT_BTSTACK_INPUT_REPORT_TIMER_OK;
}

static int scheduler_send_callback(void *context, uint16_t hid_cid, const uint8_t *report,
                                   size_t report_size) {
    swbt_btstack_input_report_timer_adapter_t *adapter =
        (swbt_btstack_input_report_timer_adapter_t *)context;
    if (adapter == NULL || report == NULL || report_size > UINT16_MAX) {
        return -1;
    }
    return send_hidp_input_report(adapter, hid_cid, report, report_size);
}

static int reply_queue_send_callback(void *context, uint16_t hid_cid, const uint8_t *report,
                                     size_t report_size) {
    swbt_btstack_input_report_timer_adapter_t *adapter =
        (swbt_btstack_input_report_timer_adapter_t *)context;
    if (adapter == NULL || report == NULL || report_size > UINT16_MAX) {
        return -1;
    }
    return send_hidp_input_report(adapter, hid_cid, report, report_size);
}

static void adapter_timer_handler(btstack_timer_source_t *timer) {
    swbt_btstack_input_report_timer_adapter_t *adapter =
        (swbt_btstack_input_report_timer_adapter_t *)timer->context;
    (void)swbt_btstack_input_report_timer_adapter_on_timer(adapter);
}

static void backend_set_timer_handler(btstack_timer_source_t *timer,
                                      void (*process)(btstack_timer_source_t *timer)) {
    (void)swbt_btstack_timer_port_set_handler(swbt_btstack_timer_port_btstack(), timer, process);
}

static void backend_set_timer_context(btstack_timer_source_t *timer, void *context) {
    (void)swbt_btstack_timer_port_set_context(swbt_btstack_timer_port_btstack(), timer, context);
}

static void backend_set_timer(btstack_timer_source_t *timer, uint32_t timeout_ms) {
    (void)swbt_btstack_timer_port_set_timer(swbt_btstack_timer_port_btstack(), timer, timeout_ms);
}

static void backend_add_timer(btstack_timer_source_t *timer) {
    (void)swbt_btstack_timer_port_add_timer(swbt_btstack_timer_port_btstack(), timer);
}

static int backend_remove_timer(btstack_timer_source_t *timer) {
    return swbt_btstack_timer_port_remove_timer(swbt_btstack_timer_port_btstack(), timer) ==
                   SWBT_BTSTACK_TIMER_PORT_OK
               ? 1
               : 0;
}

static uint32_t backend_get_time_ms(void) {
    return swbt_btstack_timer_port_get_time_ms(swbt_btstack_timer_port_btstack());
}

static void backend_request_can_send_now_event(uint16_t hid_cid) {
    (void)swbt_btstack_hid_port_request_can_send_now(swbt_btstack_hid_port_btstack(), hid_cid);
}

static int backend_send_interrupt_message(uint16_t hid_cid, const uint8_t *message,
                                          uint16_t message_len) {
    return swbt_btstack_hid_port_send_report(swbt_btstack_hid_port_btstack(), hid_cid, message,
                                             message_len) == SWBT_BTSTACK_HID_PORT_OK
               ? 0
               : -1;
}

const swbt_btstack_input_report_timer_backend_t *
swbt_btstack_input_report_timer_backend_btstack(void) {
    static const swbt_btstack_input_report_timer_backend_t backend = {
        .set_timer_handler = backend_set_timer_handler,
        .set_timer_context = backend_set_timer_context,
        .set_timer = backend_set_timer,
        .add_timer = backend_add_timer,
        .remove_timer = backend_remove_timer,
        .get_time_ms = backend_get_time_ms,
        .request_can_send_now_event = backend_request_can_send_now_event,
        .send_interrupt_message = backend_send_interrupt_message,
    };
    return &backend;
}

swbt_btstack_input_report_timer_result_t swbt_btstack_input_report_timer_adapter_init(
    swbt_btstack_input_report_timer_adapter_t *adapter,
    const swbt_btstack_input_report_timer_adapter_config_t *config) {
    if (adapter == NULL || config == NULL || !backend_is_valid(config->backend) ||
        config->state_provider == NULL) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT;
    }

    *adapter = (swbt_btstack_input_report_timer_adapter_t){0};
    adapter->backend = config->backend;
    adapter->state_provider = config->state_provider;
    adapter->state_context = config->state_context;
    adapter->report_tick_observer = config->report_tick_observer;
    adapter->report_tick_context = config->report_tick_context;

    if (swbt_btstack_subcommand_reply_queue_init(&adapter->reply_queue) !=
            SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK ||
        swbt_btstack_input_report_scheduler_init(&adapter->scheduler, scheduler_send_callback,
                                                 adapter, &config->scheduler_config) !=
            SWBT_BTSTACK_INPUT_REPORT_OK) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT;
    }

    adapter->initialized = true;
    return SWBT_BTSTACK_INPUT_REPORT_TIMER_OK;
}

swbt_btstack_input_report_timer_result_t swbt_btstack_input_report_timer_adapter_start(
    swbt_btstack_input_report_timer_adapter_t *adapter,
    swbt_btstack_input_report_timer_start_options_t options) {
    if (adapter == NULL || !adapter->initialized) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT;
    }

    const swbt_btstack_input_report_result_t start_result =
        swbt_btstack_input_report_scheduler_start(&adapter->scheduler, options.hid_cid,
                                                  options.now_us);
    if (start_result != SWBT_BTSTACK_INPUT_REPORT_OK) {
        return map_scheduler_result(start_result);
    }

    adapter->hid_cid = options.hid_cid;
    adapter->running = true;
    adapter->timer_pending = false;
    adapter->can_send_pending = false;
    adapter->periodic_holdoff_until_us = 0u;
    adapter->neutral_send_pending = false;
    adapter->backend->set_timer_handler(&adapter->timer, adapter_timer_handler);
    adapter->backend->set_timer_context(&adapter->timer, adapter);
    return schedule_next_timer(adapter, options.now_us);
}

swbt_btstack_input_report_timer_result_t swbt_btstack_input_report_timer_adapter_on_timer(
    swbt_btstack_input_report_timer_adapter_t *adapter) {
    if (adapter == NULL) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT;
    }
    if (!adapter->running) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_STOPPED;
    }
    const uint64_t now_us = (uint64_t)adapter->backend->get_time_ms() * 1000u;
    if (now_us < adapter->periodic_holdoff_until_us) {
        adapter->timer_pending = false;
        return schedule_next_timer(adapter, now_us);
    }

    adapter->timer_pending = false;
    adapter->can_send_pending = true;
    adapter->backend->request_can_send_now_event(adapter->hid_cid);
    return SWBT_BTSTACK_INPUT_REPORT_TIMER_OK;
}

swbt_btstack_input_report_timer_result_t swbt_btstack_input_report_timer_adapter_on_can_send_now(
    swbt_btstack_input_report_timer_adapter_t *adapter) {
    if (adapter == NULL) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT;
    }
    if (!adapter->running) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_STOPPED;
    }
    if (adapter->neutral_send_pending) {
        const swbt_btstack_input_report_timer_result_t neutral_result =
            send_neutral_report(adapter);
        if (neutral_result != SWBT_BTSTACK_INPUT_REPORT_TIMER_OK) {
            adapter->backend->request_can_send_now_event(adapter->hid_cid);
            return neutral_result;
        }

        const uint64_t now_us = (uint64_t)adapter->backend->get_time_ms() * 1000u;
        adapter->neutral_send_pending = false;
        adapter->can_send_pending = false;
        adapter->periodic_holdoff_until_us = 0u;
        cancel_pending_timer(adapter);
        return schedule_next_timer(adapter, now_us);
    }
    if (swbt_btstack_subcommand_reply_queue_size(&adapter->reply_queue) > 0u) {
        const swbt_btstack_subcommand_reply_queue_result_t reply_result =
            swbt_btstack_subcommand_reply_queue_send_next(&adapter->reply_queue,
                                                          reply_queue_send_callback, adapter);
        if (reply_result != SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK) {
            return map_reply_queue_result(reply_result);
        }
        const uint64_t now_us = (uint64_t)adapter->backend->get_time_ms() * 1000u;
        const swbt_btstack_input_report_timer_result_t holdoff_result =
            holdoff_periodic_after_reply(adapter, now_us);
        if (holdoff_result != SWBT_BTSTACK_INPUT_REPORT_TIMER_OK) {
            return holdoff_result;
        }
        if (swbt_btstack_subcommand_reply_queue_size(&adapter->reply_queue) > 0u ||
            adapter->can_send_pending) {
            adapter->backend->request_can_send_now_event(adapter->hid_cid);
        }
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_OK;
    }

    if (!adapter->can_send_pending) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_NOT_DUE;
    }
    const uint64_t now_us = (uint64_t)adapter->backend->get_time_ms() * 1000u;
    if (now_us < adapter->periodic_holdoff_until_us) {
        adapter->can_send_pending = false;
        return schedule_next_timer(adapter, now_us);
    }

    const swbt_state_t state = adapter->state_provider(adapter->state_context);
    const swbt_btstack_input_report_result_t tick_result =
        swbt_btstack_input_report_scheduler_tick(&adapter->scheduler, now_us, &state);
    notify_report_tick(adapter, now_us, tick_result);
    const swbt_btstack_input_report_timer_result_t result = map_scheduler_result(tick_result);
    adapter->can_send_pending = false;
    if (result != SWBT_BTSTACK_INPUT_REPORT_TIMER_OK) {
        return result;
    }
    adapter->periodic_holdoff_until_us = 0u;
    return schedule_next_timer(adapter, now_us);
}

swbt_btstack_input_report_timer_result_t
swbt_btstack_input_report_timer_adapter_enqueue_subcommand_reply(
    swbt_btstack_input_report_timer_adapter_t *adapter, uint16_t hid_cid, const uint8_t *report,
    size_t report_size) {
    if (adapter == NULL || !adapter->initialized) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT;
    }
    if (!adapter->running) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_STOPPED;
    }

    const swbt_btstack_subcommand_reply_queue_result_t enqueue_result =
        swbt_btstack_subcommand_reply_queue_enqueue(&adapter->reply_queue, hid_cid, report,
                                                    report_size);
    if (enqueue_result != SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK) {
        return map_reply_queue_result(enqueue_result);
    }

    adapter->backend->request_can_send_now_event(hid_cid);
    return SWBT_BTSTACK_INPUT_REPORT_TIMER_OK;
}

swbt_btstack_input_report_timer_result_t swbt_btstack_input_report_timer_adapter_send_neutral_now(
    swbt_btstack_input_report_timer_adapter_t *adapter) {
    if (adapter == NULL) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT;
    }
    if (!adapter->running) {
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_STOPPED;
    }

    const swbt_btstack_input_report_timer_result_t result = send_neutral_report(adapter);
    if (result != SWBT_BTSTACK_INPUT_REPORT_TIMER_OK) {
        adapter->neutral_send_pending = true;
        adapter->backend->request_can_send_now_event(adapter->hid_cid);
        return SWBT_BTSTACK_INPUT_REPORT_TIMER_PENDING;
    }

    adapter->neutral_send_pending = false;
    return SWBT_BTSTACK_INPUT_REPORT_TIMER_OK;
}

void swbt_btstack_input_report_timer_adapter_stop(
    swbt_btstack_input_report_timer_adapter_t *adapter) {
    if (adapter == NULL) {
        return;
    }
    if (adapter->timer_pending) {
        (void)adapter->backend->remove_timer(&adapter->timer);
        adapter->timer_pending = false;
    }
    adapter->can_send_pending = false;
    adapter->neutral_send_pending = false;
    adapter->periodic_holdoff_until_us = 0u;
    adapter->running = false;
    (void)swbt_btstack_subcommand_reply_queue_init(&adapter->reply_queue);
    swbt_btstack_input_report_scheduler_stop(&adapter->scheduler);
}
