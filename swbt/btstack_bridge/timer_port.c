#include "btstack_bridge/timer_port.h"

#include <stdbool.h>
#include <stddef.h>

static bool swbt_btstack_timer_port_is_valid(const swbt_btstack_timer_port_t *port) {
    return port != NULL && port->set_timer_handler != NULL && port->set_timer_context != NULL &&
           port->set_timer != NULL && port->add_timer != NULL && port->remove_timer != NULL &&
           port->get_time_ms != NULL;
}

const swbt_btstack_timer_port_t *swbt_btstack_timer_port_btstack(void) {
    static const swbt_btstack_timer_port_t port = {
        .set_timer_handler = btstack_run_loop_set_timer_handler,
        .set_timer_context = btstack_run_loop_set_timer_context,
        .set_timer = btstack_run_loop_set_timer,
        .add_timer = btstack_run_loop_add_timer,
        .remove_timer = btstack_run_loop_remove_timer,
        .get_time_ms = btstack_run_loop_get_time_ms,
    };
    return &port;
}

swbt_btstack_timer_port_result_t
swbt_btstack_timer_port_set_handler(const swbt_btstack_timer_port_t *port,
                                    btstack_timer_source_t *timer,
                                    void (*process)(btstack_timer_source_t *timer)) {
    if (!swbt_btstack_timer_port_is_valid(port) || timer == NULL || process == NULL) {
        return SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT;
    }

    port->set_timer_handler(timer, process);
    return SWBT_BTSTACK_TIMER_PORT_OK;
}

swbt_btstack_timer_port_result_t
swbt_btstack_timer_port_set_context(const swbt_btstack_timer_port_t *port,
                                    btstack_timer_source_t *timer, void *context) {
    if (!swbt_btstack_timer_port_is_valid(port) || timer == NULL) {
        return SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT;
    }

    port->set_timer_context(timer, context);
    return SWBT_BTSTACK_TIMER_PORT_OK;
}

swbt_btstack_timer_port_result_t
swbt_btstack_timer_port_set_timer(const swbt_btstack_timer_port_t *port,
                                  btstack_timer_source_t *timer, uint32_t timeout_ms) {
    if (!swbt_btstack_timer_port_is_valid(port) || timer == NULL) {
        return SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT;
    }

    port->set_timer(timer, timeout_ms);
    return SWBT_BTSTACK_TIMER_PORT_OK;
}

swbt_btstack_timer_port_result_t
swbt_btstack_timer_port_add_timer(const swbt_btstack_timer_port_t *port,
                                  btstack_timer_source_t *timer) {
    if (!swbt_btstack_timer_port_is_valid(port) || timer == NULL) {
        return SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT;
    }

    port->add_timer(timer);
    return SWBT_BTSTACK_TIMER_PORT_OK;
}

swbt_btstack_timer_port_result_t
swbt_btstack_timer_port_remove_timer(const swbt_btstack_timer_port_t *port,
                                     btstack_timer_source_t *timer) {
    if (!swbt_btstack_timer_port_is_valid(port) || timer == NULL) {
        return SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT;
    }

    return port->remove_timer(timer) != 0 ? SWBT_BTSTACK_TIMER_PORT_OK
                                          : SWBT_BTSTACK_TIMER_PORT_ERROR_REMOVE_FAILED;
}

uint32_t swbt_btstack_timer_port_get_time_ms(const swbt_btstack_timer_port_t *port) {
    if (!swbt_btstack_timer_port_is_valid(port)) {
        return 0u;
    }

    return port->get_time_ms();
}
