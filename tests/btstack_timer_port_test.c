#include "btstack_bridge/timer_port.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    bool set_handler_called;
    bool set_context_called;
    int set_timer_calls;
    int add_timer_calls;
    int remove_timer_calls;
    btstack_timer_source_t *timer;
    void (*timer_handler)(btstack_timer_source_t *timer);
    void *timer_context;
    uint32_t timeout_ms;
    uint32_t now_ms;
} fake_btstack_t;

static fake_btstack_t g_fake_btstack;

void btstack_run_loop_set_timer_handler(btstack_timer_source_t *timer,
                                        void (*process)(btstack_timer_source_t *timer)) {
    g_fake_btstack.set_handler_called = true;
    g_fake_btstack.timer = timer;
    g_fake_btstack.timer_handler = process;
}

void btstack_run_loop_set_timer_context(btstack_timer_source_t *timer, void *context) {
    g_fake_btstack.set_context_called = true;
    g_fake_btstack.timer = timer;
    g_fake_btstack.timer_context = context;
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

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static void sample_timer_handler(btstack_timer_source_t *timer) {
    (void)timer;
}

static int btstack_timer_port_forwards_run_loop_operations(void) {
    const swbt_btstack_timer_port_t *port = swbt_btstack_timer_port_btstack();
    btstack_timer_source_t timer = {0};
    int context = 7;

    fake_reset(123u);
    int failed = 0;
    failed += expect_true(port != NULL);
    failed += expect_eq_int(swbt_btstack_timer_port_set_handler(port, &timer, sample_timer_handler),
                            SWBT_BTSTACK_TIMER_PORT_OK);
    failed += expect_eq_int(swbt_btstack_timer_port_set_context(port, &timer, &context),
                            SWBT_BTSTACK_TIMER_PORT_OK);
    failed += expect_eq_int(swbt_btstack_timer_port_set_timer(port, &timer, 25u),
                            SWBT_BTSTACK_TIMER_PORT_OK);
    failed +=
        expect_eq_int(swbt_btstack_timer_port_add_timer(port, &timer), SWBT_BTSTACK_TIMER_PORT_OK);
    failed += expect_eq_int(swbt_btstack_timer_port_remove_timer(port, &timer),
                            SWBT_BTSTACK_TIMER_PORT_OK);
    failed += expect_eq_u32(swbt_btstack_timer_port_get_time_ms(port), 123u);
    failed += expect_true(g_fake_btstack.set_handler_called);
    failed += expect_true(g_fake_btstack.set_context_called);
    failed += expect_true(g_fake_btstack.timer_handler == sample_timer_handler);
    failed += expect_true(g_fake_btstack.timer_context == &context);
    failed += expect_eq_int(g_fake_btstack.set_timer_calls, 1);
    failed += expect_eq_int(g_fake_btstack.add_timer_calls, 1);
    failed += expect_eq_int(g_fake_btstack.remove_timer_calls, 1);
    failed += expect_eq_u32(g_fake_btstack.timeout_ms, 25u);
    return failed;
}

static int invalid_arguments_are_rejected_before_btstack_calls(void) {
    btstack_timer_source_t timer = {0};
    fake_reset(0u);
    int failed = 0;
    failed += expect_eq_int(swbt_btstack_timer_port_set_handler(NULL, &timer, sample_timer_handler),
                            SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_timer_port_set_handler(swbt_btstack_timer_port_btstack(),
                                                                NULL, sample_timer_handler),
                            SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(
        swbt_btstack_timer_port_set_handler(swbt_btstack_timer_port_btstack(), &timer, NULL),
        SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_timer_port_set_context(NULL, &timer, NULL),
                            SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_timer_port_set_timer(NULL, &timer, 1u),
                            SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_timer_port_add_timer(NULL, &timer),
                            SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_timer_port_remove_timer(NULL, &timer),
                            SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_u32(swbt_btstack_timer_port_get_time_ms(NULL), 0u);
    failed += expect_eq_int(g_fake_btstack.set_timer_calls, 0);
    failed += expect_eq_int(g_fake_btstack.add_timer_calls, 0);
    failed += expect_eq_int(g_fake_btstack.remove_timer_calls, 0);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += btstack_timer_port_forwards_run_loop_operations();
    failed += invalid_arguments_are_rejected_before_btstack_calls();
    return failed == 0 ? 0 : 1;
}
