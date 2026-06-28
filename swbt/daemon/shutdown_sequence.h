#ifndef SWBT_DAEMON_SHUTDOWN_SEQUENCE_H
#define SWBT_DAEMON_SHUTDOWN_SEQUENCE_H

#include <stdatomic.h>
#include <stdbool.h>

#include "btstack_bridge/production_ports.h"
#include "daemon/process.h"
#include "daemon/shutdown_listener.h"

typedef void (*swbt_daemon_shutdown_sequence_finish_t)(void *context);

typedef struct {
    const swbt_btstack_production_run_loop_port_t *run_loop;
    void *port_context;
    swbt_daemon_process_t **host;
    swbt_daemon_shutdown_sequence_finish_t finish;
    void *finish_context;
} swbt_daemon_shutdown_sequence_config_t;

typedef struct {
    const swbt_btstack_production_run_loop_port_t *run_loop;
    void *port_context;
    swbt_daemon_process_t **host;
    swbt_daemon_shutdown_sequence_finish_t finish;
    void *finish_context;
    bool neutral_pending;
    btstack_context_callback_registration_t callback;
    atomic_bool requested;
} swbt_daemon_shutdown_sequence_t;

bool swbt_daemon_shutdown_sequence_listener_is_valid(
    const swbt_daemon_shutdown_listener_t *shutdown_listener);

bool swbt_daemon_shutdown_sequence_init(swbt_daemon_shutdown_sequence_t *shutdown,
                                        const swbt_daemon_shutdown_sequence_config_t *config);

void swbt_daemon_shutdown_sequence_prepare(swbt_daemon_shutdown_sequence_t *shutdown);

int swbt_daemon_shutdown_sequence_install_listener(
    swbt_daemon_shutdown_sequence_t *shutdown,
    const swbt_daemon_shutdown_listener_t *shutdown_listener, void *shutdown_context);

void swbt_daemon_shutdown_sequence_uninstall_listener(
    const swbt_daemon_shutdown_listener_t *shutdown_listener, void *shutdown_context);

void swbt_daemon_shutdown_sequence_request(void *context);

void swbt_daemon_shutdown_sequence_finish(swbt_daemon_shutdown_sequence_t *shutdown);

#endif
