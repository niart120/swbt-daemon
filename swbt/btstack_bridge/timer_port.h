#ifndef SWBT_BTSTACK_BRIDGE_TIMER_PORT_H
#define SWBT_BTSTACK_BRIDGE_TIMER_PORT_H

#include <stdint.h>

#include "btstack_run_loop.h"

typedef enum {
    SWBT_BTSTACK_TIMER_PORT_OK = 0,
    SWBT_BTSTACK_TIMER_PORT_ERROR_INVALID_ARGUMENT = -1,
    SWBT_BTSTACK_TIMER_PORT_ERROR_REMOVE_FAILED = -2,
} swbt_btstack_timer_port_result_t;

typedef struct {
    void (*set_timer_handler)(btstack_timer_source_t *timer,
                              void (*process)(btstack_timer_source_t *timer));
    void (*set_timer_context)(btstack_timer_source_t *timer, void *context);
    void (*set_timer)(btstack_timer_source_t *timer, uint32_t timeout_ms);
    void (*add_timer)(btstack_timer_source_t *timer);
    int (*remove_timer)(btstack_timer_source_t *timer);
    uint32_t (*get_time_ms)(void);
} swbt_btstack_timer_port_t;

const swbt_btstack_timer_port_t *swbt_btstack_timer_port_btstack(void);

swbt_btstack_timer_port_result_t
swbt_btstack_timer_port_set_handler(const swbt_btstack_timer_port_t *port,
                                    btstack_timer_source_t *timer,
                                    void (*process)(btstack_timer_source_t *timer));

swbt_btstack_timer_port_result_t
swbt_btstack_timer_port_set_context(const swbt_btstack_timer_port_t *port,
                                    btstack_timer_source_t *timer, void *context);

swbt_btstack_timer_port_result_t
swbt_btstack_timer_port_set_timer(const swbt_btstack_timer_port_t *port,
                                  btstack_timer_source_t *timer, uint32_t timeout_ms);

swbt_btstack_timer_port_result_t
swbt_btstack_timer_port_add_timer(const swbt_btstack_timer_port_t *port,
                                  btstack_timer_source_t *timer);

swbt_btstack_timer_port_result_t
swbt_btstack_timer_port_remove_timer(const swbt_btstack_timer_port_t *port,
                                     btstack_timer_source_t *timer);

uint32_t swbt_btstack_timer_port_get_time_ms(const swbt_btstack_timer_port_t *port);

#endif
