#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include "btstack_bridge/input_report_timer_adapter.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_report.h"
#include "switch/switch_subcommand_reply.h"

typedef struct {
    bool set_handler_called;
    bool set_context_called;
    int set_timer_calls;
    int add_timer_calls;
    int remove_timer_calls;
    int request_can_send_calls;
    int send_interrupt_calls;
    btstack_timer_source_t *timer;
    void (*timer_handler)(btstack_timer_source_t *timer);
    void *timer_context;
    uint32_t timeout_ms;
    uint32_t now_ms;
    int send_interrupt_result;
    uint16_t hid_cid;
    uint8_t report[1u + SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE];
    uint16_t report_size;
} fake_btstack_t;

static fake_btstack_t g_fake_btstack;

void btstack_run_loop_set_timer_handler(btstack_timer_source_t *timer,
                                        void (*process)(btstack_timer_source_t *timer)) {
    g_fake_btstack.set_handler_called = true;
    g_fake_btstack.timer = timer;
    g_fake_btstack.timer_handler = process;
    timer->process = process;
}

void btstack_run_loop_set_timer_context(btstack_timer_source_t *timer, void *context) {
    g_fake_btstack.set_context_called = true;
    g_fake_btstack.timer = timer;
    g_fake_btstack.timer_context = context;
    timer->context = context;
}

void btstack_run_loop_set_timer(btstack_timer_source_t *timer, uint32_t timeout_in_ms) {
    g_fake_btstack.timer = timer;
    g_fake_btstack.timeout_ms = timeout_in_ms;
    g_fake_btstack.set_timer_calls += 1;
}

void btstack_run_loop_add_timer(btstack_timer_source_t *timer) {
    g_fake_btstack.timer = timer;
    g_fake_btstack.add_timer_calls += 1;
}

int btstack_run_loop_remove_timer(btstack_timer_source_t *timer) {
    g_fake_btstack.timer = timer;
    g_fake_btstack.remove_timer_calls += 1;
    return 1;
}

uint32_t btstack_run_loop_get_time_ms(void) {
    return g_fake_btstack.now_ms;
}

void hid_device_request_can_send_now_event(uint16_t hid_cid) {
    g_fake_btstack.hid_cid = hid_cid;
    g_fake_btstack.request_can_send_calls += 1;
}

void hid_device_send_interrupt_message(uint16_t hid_cid, const uint8_t *message,
                                       uint16_t message_len) {
    g_fake_btstack.hid_cid = hid_cid;
    g_fake_btstack.report_size = message_len;
    g_fake_btstack.send_interrupt_calls += 1;
    for (uint16_t index = 0; index < message_len && index < sizeof(g_fake_btstack.report);
         ++index) {
        g_fake_btstack.report[index] = message[index];
    }
}

static void fake_reset(uint32_t now_ms) {
    g_fake_btstack = (fake_btstack_t){0};
    g_fake_btstack.now_ms = now_ms;
}

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static swbt_state_t sample_state(void) {
    swbt_state_t state = swbt_state_neutral();
    state.buttons = 0x00422408u;
    state.lx = 0x0123u;
    state.ly = 0x0ABCu;
    state.rx = 0x0FFFu;
    state.ry = 0x000Fu;
    return state;
}

static swbt_state_t provide_state(void *context) {
    return *(const swbt_state_t *)context;
}

static swbt_btstack_input_report_timer_adapter_config_t sample_config(swbt_state_t *state) {
    swbt_btstack_input_report_timer_adapter_config_t config = {
        .backend = swbt_btstack_input_report_timer_backend_btstack(),
        .state_provider = provide_state,
        .state_context = state,
        .scheduler_config =
            {
                .report_period_us = SWBT_BTSTACK_INPUT_REPORT_DEFAULT_PERIOD_US,
                .report_options =
                    {
                        .timer = 0x41u,
                        .battery_connection = 0x8Eu,
                        .vibrator_report = 0x80u,
                    },
            },
    };
    return config;
}

static swbt_btstack_input_report_timer_start_options_t start_options(uint16_t hid_cid,
                                                                     uint64_t now_us) {
    swbt_btstack_input_report_timer_start_options_t options = {
        .hid_cid = hid_cid,
        .now_us = now_us,
    };
    return options;
}

static int init_adapter(swbt_btstack_input_report_timer_adapter_t *adapter, swbt_state_t *state) {
    const swbt_btstack_input_report_timer_adapter_config_t config = sample_config(state);
    return expect_eq_int(swbt_btstack_input_report_timer_adapter_init(adapter, &config),
                         SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
}

static int start_registers_timer_for_first_due_tick(void) {
    swbt_btstack_input_report_timer_adapter_t adapter;
    swbt_state_t state = sample_state();

    fake_reset(1u);
    int failed = 0;
    failed += init_adapter(&adapter, &state);
    failed += expect_eq_int(
        swbt_btstack_input_report_timer_adapter_start(&adapter, start_options(0x0042u, 1000u)),
        SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_true(g_fake_btstack.set_handler_called);
    failed += expect_true(g_fake_btstack.set_context_called);
    failed += expect_true(g_fake_btstack.timer_context == &adapter);
    failed += expect_eq_int(g_fake_btstack.set_timer_calls, 1);
    failed += expect_eq_int(g_fake_btstack.add_timer_calls, 1);
    failed += expect_eq_u32(g_fake_btstack.timeout_ms, 8u);
    return failed;
}

static int start_does_not_send_periodic_until_timer_requests_can_send(void) {
    swbt_btstack_input_report_timer_adapter_t adapter;
    swbt_state_t state = sample_state();

    fake_reset(1u);
    int failed = 0;
    failed += init_adapter(&adapter, &state);
    failed += expect_eq_int(
        swbt_btstack_input_report_timer_adapter_start(&adapter, start_options(0x0042u, 1000u)),
        SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_true(g_fake_btstack.set_handler_called);
    failed += expect_true(g_fake_btstack.set_context_called);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_NOT_DUE);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 0);
    return failed;
}

static void fill_reply(uint8_t *report, uint8_t seed) {
    for (size_t index = 0; index < SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE; ++index) {
        report[index] = (uint8_t)(seed + index);
    }
    report[0] = SWBT_SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY;
}

static int timer_callback_requests_can_send_without_sending(void) {
    swbt_btstack_input_report_timer_adapter_t adapter;
    swbt_state_t state = sample_state();

    fake_reset(1u);
    int failed = 0;
    failed += init_adapter(&adapter, &state);
    failed += expect_eq_int(
        swbt_btstack_input_report_timer_adapter_start(&adapter, start_options(0x0042u, 1000u)),
        SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);

    g_fake_btstack.timer_handler(g_fake_btstack.timer);
    failed += expect_eq_int(g_fake_btstack.request_can_send_calls, 1);
    failed += expect_eq_u16(g_fake_btstack.hid_cid, 0x0042u);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 0);
    return failed;
}

static int can_send_callback_sends_one_report_and_schedules_next_timer(void) {
    swbt_btstack_input_report_timer_adapter_t adapter;
    swbt_state_t state = sample_state();

    fake_reset(1u);
    int failed = 0;
    failed += init_adapter(&adapter, &state);
    failed += expect_eq_int(
        swbt_btstack_input_report_timer_adapter_start(&adapter, start_options(0x0042u, 1000u)),
        SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    g_fake_btstack.timer_handler(g_fake_btstack.timer);

    g_fake_btstack.now_ms = 9u;
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 1);
    failed += expect_eq_u16(g_fake_btstack.hid_cid, 0x0042u);
    failed += expect_eq_u16(g_fake_btstack.report_size, 1u + SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE);
    failed += expect_eq_u8(g_fake_btstack.report[0], 0xA1u);
    failed += expect_eq_u8(g_fake_btstack.report[1], SWBT_SWITCH_INPUT_REPORT_STANDARD_FULL);
    failed += expect_eq_u8(g_fake_btstack.report[2], 0x41u);
    failed += expect_eq_u8(g_fake_btstack.report[3], 0x8Eu);
    failed += expect_eq_u8(g_fake_btstack.report[13], 0x80u);
    failed += expect_eq_int(g_fake_btstack.add_timer_calls, 2);
    failed += expect_eq_u32(g_fake_btstack.timeout_ms, 8u);
    return failed;
}

static int fake_backend_send_interrupt_message(uint16_t hid_cid, const uint8_t *message,
                                               uint16_t message_len) {
    hid_device_send_interrupt_message(hid_cid, message, message_len);
    return g_fake_btstack.send_interrupt_result;
}

static swbt_btstack_input_report_timer_backend_t fake_backend_with_send_result(void) {
    swbt_btstack_input_report_timer_backend_t backend =
        *swbt_btstack_input_report_timer_backend_btstack();
    backend.send_interrupt_message = fake_backend_send_interrupt_message;
    return backend;
}

static swbt_btstack_input_report_timer_adapter_config_t
sample_config_with_backend(swbt_state_t *state,
                           const swbt_btstack_input_report_timer_backend_t *backend) {
    swbt_btstack_input_report_timer_adapter_config_t config = sample_config(state);
    config.backend = backend;
    return config;
}

static int queued_reply_is_sent_before_pending_periodic_report(void) {
    uint8_t reply[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE] = {0};
    swbt_btstack_input_report_timer_adapter_t adapter;
    swbt_state_t state = sample_state();

    fill_reply(reply, 0x21u);
    fake_reset(1u);
    int failed = 0;
    failed += init_adapter(&adapter, &state);
    failed += expect_eq_int(
        swbt_btstack_input_report_timer_adapter_start(&adapter, start_options(0x0042u, 1000u)),
        SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    g_fake_btstack.timer_handler(g_fake_btstack.timer);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_enqueue_subcommand_reply(
                                &adapter, 0x0042u, reply, sizeof(reply)),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);

    g_fake_btstack.now_ms = 9u;
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 1);
    failed += expect_eq_u8(g_fake_btstack.report[0], 0xA1u);
    failed += expect_eq_u8(g_fake_btstack.report[1], SWBT_SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY);
    failed +=
        expect_eq_u16(g_fake_btstack.report_size, 1u + SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    failed += expect_eq_int(g_fake_btstack.request_can_send_calls, 2);
    failed += expect_eq_int(g_fake_btstack.remove_timer_calls, 0);
    failed += expect_eq_u32(g_fake_btstack.timeout_ms, 300u);
    if (failed != 0) {
        (void)printf("after reply send=%d request=%d remove=%d timeout=%u add=%d\n",
                     g_fake_btstack.send_interrupt_calls, g_fake_btstack.request_can_send_calls,
                     g_fake_btstack.remove_timer_calls, g_fake_btstack.timeout_ms,
                     g_fake_btstack.add_timer_calls);
    }

    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_NOT_DUE);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 1);

    g_fake_btstack.now_ms = 309u;
    g_fake_btstack.timer_handler(g_fake_btstack.timer);
    failed += expect_eq_int(g_fake_btstack.request_can_send_calls, 3);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 2);
    failed += expect_eq_u8(g_fake_btstack.report[0], 0xA1u);
    failed += expect_eq_u8(g_fake_btstack.report[1], SWBT_SWITCH_INPUT_REPORT_STANDARD_FULL);
    failed += expect_eq_int(g_fake_btstack.add_timer_calls, 3);
    if (failed != 0) {
        (void)printf("final send=%d request=%d remove=%d timeout=%u add=%d report=%02x %02x\n",
                     g_fake_btstack.send_interrupt_calls, g_fake_btstack.request_can_send_calls,
                     g_fake_btstack.remove_timer_calls, g_fake_btstack.timeout_ms,
                     g_fake_btstack.add_timer_calls, g_fake_btstack.report[0],
                     g_fake_btstack.report[1]);
    }
    return failed;
}

static int queued_replies_share_advancing_input_report_timer(void) {
    uint8_t reply[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE] = {0};
    swbt_btstack_input_report_timer_adapter_t adapter;
    swbt_state_t state = sample_state();

    fill_reply(reply, 0x21u);
    reply[1] = 0x00u;
    fake_reset(1u);

    int failed = 0;
    failed += init_adapter(&adapter, &state);
    failed += expect_eq_int(
        swbt_btstack_input_report_timer_adapter_start(&adapter, start_options(0x0042u, 1000u)),
        SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);

    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_enqueue_subcommand_reply(
                                &adapter, 0x0042u, reply, sizeof(reply)),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 1);
    failed += expect_eq_u8(g_fake_btstack.report[0], 0xA1u);
    failed += expect_eq_u8(g_fake_btstack.report[1], SWBT_SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY);
    failed += expect_eq_u8(g_fake_btstack.report[2], 0x41u);

    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_enqueue_subcommand_reply(
                                &adapter, 0x0042u, reply, sizeof(reply)),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 2);
    failed += expect_eq_u8(g_fake_btstack.report[1], SWBT_SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY);
    failed += expect_eq_u8(g_fake_btstack.report[2], 0x42u);

    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_enqueue_subcommand_reply(
                                &adapter, 0x0042u, reply, sizeof(reply)),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 3);
    failed += expect_eq_u8(g_fake_btstack.report[1], SWBT_SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY);
    failed += expect_eq_u8(g_fake_btstack.report[2], 0x43u);

    g_fake_btstack.now_ms = 301u;
    g_fake_btstack.timer_handler(g_fake_btstack.timer);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 4);
    failed += expect_eq_u8(g_fake_btstack.report[1], SWBT_SWITCH_INPUT_REPORT_STANDARD_FULL);
    failed += expect_eq_u8(g_fake_btstack.report[2], 0x44u);
    return failed;
}

static int reply_send_failure_keeps_item_for_retry(void) {
    uint8_t reply[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE] = {0};
    swbt_btstack_input_report_timer_adapter_t adapter;
    swbt_state_t state = sample_state();
    const swbt_btstack_input_report_timer_backend_t backend = fake_backend_with_send_result();
    const swbt_btstack_input_report_timer_adapter_config_t config =
        sample_config_with_backend(&state, &backend);

    fill_reply(reply, 0x31u);
    fake_reset(1u);
    int failed = 0;
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_init(&adapter, &config),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(
        swbt_btstack_input_report_timer_adapter_start(&adapter, start_options(0x0043u, 1000u)),
        SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_enqueue_subcommand_reply(
                                &adapter, 0x0043u, reply, sizeof(reply)),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);

    g_fake_btstack.send_interrupt_result = -7;
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_REPLY_QUEUE);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 1);

    g_fake_btstack.send_interrupt_result = 0;
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 2);
    failed += expect_eq_u8(g_fake_btstack.report[0], 0xA1u);
    failed += expect_eq_u8(g_fake_btstack.report[1], SWBT_SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY);
    return failed;
}

static int stop_cancels_timer_and_prevents_later_sends(void) {
    swbt_btstack_input_report_timer_adapter_t adapter;
    swbt_state_t state = sample_state();

    fake_reset(1u);
    int failed = 0;
    failed += init_adapter(&adapter, &state);
    failed += expect_eq_int(
        swbt_btstack_input_report_timer_adapter_start(&adapter, start_options(0x0042u, 1000u)),
        SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    swbt_btstack_input_report_timer_adapter_stop(&adapter);
    failed += expect_eq_int(g_fake_btstack.remove_timer_calls, 1);

    const int request_can_send_calls = g_fake_btstack.request_can_send_calls;
    g_fake_btstack.timer_handler(g_fake_btstack.timer);
    failed += expect_eq_int(g_fake_btstack.request_can_send_calls, request_can_send_calls);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_STOPPED);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 0);
    return failed;
}

static int stop_clears_queued_replies_before_later_start(void) {
    uint8_t reply[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE] = {0};
    swbt_btstack_input_report_timer_adapter_t adapter;
    swbt_state_t state = sample_state();

    fill_reply(reply, 0x41u);
    fake_reset(1u);
    int failed = 0;
    failed += init_adapter(&adapter, &state);
    failed += expect_eq_int(
        swbt_btstack_input_report_timer_adapter_start(&adapter, start_options(0x0044u, 1000u)),
        SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_enqueue_subcommand_reply(
                                &adapter, 0x0044u, reply, sizeof(reply)),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);

    swbt_btstack_input_report_timer_adapter_stop(&adapter);
    failed += expect_eq_int(
        swbt_btstack_input_report_timer_adapter_start(&adapter, start_options(0x0044u, 2000u)),
        SWBT_BTSTACK_INPUT_REPORT_TIMER_OK);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(&adapter),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_NOT_DUE);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 0);
    return failed;
}

static int invalid_arguments_are_rejected(void) {
    uint8_t reply[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE] = {0};
    swbt_btstack_input_report_timer_adapter_t adapter;
    swbt_state_t state = sample_state();
    swbt_btstack_input_report_timer_adapter_config_t config = sample_config(&state);

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_init(NULL, &config),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_init(&adapter, NULL),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT);

    config.backend = NULL;
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_init(&adapter, &config),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT);
    config = sample_config(&state);
    config.state_provider = NULL;
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_init(&adapter, &config),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT);

    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_timer(NULL),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_on_can_send_now(NULL),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_input_report_timer_adapter_enqueue_subcommand_reply(
                                NULL, 0x0042u, reply, sizeof(reply)),
                            SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(
        swbt_btstack_input_report_timer_adapter_start(NULL, start_options(0x0042u, 1000u)),
        SWBT_BTSTACK_INPUT_REPORT_TIMER_ERROR_INVALID_ARGUMENT);

    return failed;
}

typedef int (*test_fn_t)(void);

static int run_test(const char *name, test_fn_t fn) {
    const int failed = fn();
    if (failed != 0) {
        (void)printf("%s failed=%d\n", name, failed);
    }
    return failed;
}

int main(void) {
    int failed = 0;
    failed += run_test("start_registers_timer_for_first_due_tick",
                       start_registers_timer_for_first_due_tick);
    failed += run_test("start_does_not_send_periodic_until_timer_requests_can_send",
                       start_does_not_send_periodic_until_timer_requests_can_send);
    failed += run_test("timer_callback_requests_can_send_without_sending",
                       timer_callback_requests_can_send_without_sending);
    failed += run_test("can_send_callback_sends_one_report_and_schedules_next_timer",
                       can_send_callback_sends_one_report_and_schedules_next_timer);
    failed += run_test("queued_reply_is_sent_before_pending_periodic_report",
                       queued_reply_is_sent_before_pending_periodic_report);
    failed += run_test("queued_replies_share_advancing_input_report_timer",
                       queued_replies_share_advancing_input_report_timer);
    failed += run_test("reply_send_failure_keeps_item_for_retry",
                       reply_send_failure_keeps_item_for_retry);
    failed += run_test("stop_cancels_timer_and_prevents_later_sends",
                       stop_cancels_timer_and_prevents_later_sends);
    failed += run_test("stop_clears_queued_replies_before_later_start",
                       stop_clears_queued_replies_before_later_start);
    failed += run_test("invalid_arguments_are_rejected", invalid_arguments_are_rejected);
    return failed == 0 ? 0 : 1;
}
