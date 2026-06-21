#ifndef SWBT_DEBUG_CLIENT_H
#define SWBT_DEBUG_CLIENT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "ipc/ipc_server.h"
#include "switch/switch_controller_state.h"

typedef struct {
    uint16_t port;
    swbt_state_t state;
    uint32_t hold_ms;
    bool skip_release;
} swbt_debug_client_config_t;

typedef struct {
    int (*send)(void *context, const char *data, size_t size);
    int (*receive)(void *context, char *buffer, size_t buffer_size);
    void *context;
} swbt_debug_client_io_t;

int swbt_debug_client_send_hello(swbt_ipc_socket_t *socket);
int swbt_debug_client_send_acquire(swbt_ipc_socket_t *socket);
int swbt_debug_client_send_set_state(swbt_ipc_socket_t *socket, const char *owner_id,
                                     const swbt_state_t *state);
int swbt_debug_client_send_get_status(swbt_ipc_socket_t *socket);
int swbt_debug_client_send_release(swbt_ipc_socket_t *socket, const char *owner_id);
int swbt_debug_client_receive_response(swbt_ipc_socket_t *socket, char *buffer, size_t buffer_size);
int swbt_debug_client_response_string(const char *response, const char *key, char *out,
                                      size_t out_size);
int swbt_debug_client_run_io(const swbt_debug_client_config_t *config, swbt_debug_client_io_t *io,
                             FILE *out, FILE *err);
int swbt_debug_client_main(int argc, const char *const *argv, FILE *out, FILE *err);

#endif
