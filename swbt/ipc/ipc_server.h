#ifndef SWBT_IPC_SERVER_H
#define SWBT_IPC_SERVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ipc/ipc_json.h"
#include "ipc/ipc_session.h"

typedef enum {
    SWBT_IPC_SERVER_OK = 0,
    SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT = -1,
    SWBT_IPC_SERVER_ERROR_SOCKET = -2,
    SWBT_IPC_SERVER_ERROR_UNSUPPORTED_BIND = -3,
    SWBT_IPC_SERVER_ERROR_DISCONNECTED = -4,
    SWBT_IPC_SERVER_ERROR_MESSAGE_TOO_LONG = -5,
} swbt_ipc_server_result_t;

typedef struct {
    uintptr_t handle;
    bool open;
} swbt_ipc_socket_t;

typedef struct {
    swbt_ipc_socket_t socket;
    uint32_t client_id;
    bool open;
} swbt_ipc_connection_t;

typedef struct {
    swbt_ipc_socket_t listen_socket;
    swbt_ipc_session_t session;
    uint32_t next_client_id;
    uint16_t bound_port;
    bool listening;
} swbt_ipc_server_t;

void swbt_ipc_socket_init(swbt_ipc_socket_t *socket);
void swbt_ipc_socket_close(swbt_ipc_socket_t *socket);

swbt_ipc_server_result_t swbt_ipc_socket_connect_loopback(swbt_ipc_socket_t *socket, uint16_t port);
swbt_ipc_server_result_t swbt_ipc_socket_send_all(swbt_ipc_socket_t *socket, const char *data,
                                                  size_t size);
swbt_ipc_server_result_t swbt_ipc_socket_receive(swbt_ipc_socket_t *socket, char *buffer,
                                                 size_t buffer_size, size_t *out_received);

swbt_ipc_server_result_t swbt_ipc_server_init(swbt_ipc_server_t *server);
swbt_ipc_server_result_t swbt_ipc_server_listen(swbt_ipc_server_t *server, const char *host,
                                                uint16_t port, int backlog);
uint16_t swbt_ipc_server_port(const swbt_ipc_server_t *server);
swbt_ipc_server_result_t swbt_ipc_server_accept(swbt_ipc_server_t *server,
                                                swbt_ipc_connection_t *out_connection);
swbt_ipc_server_result_t swbt_ipc_server_serve_connection_once(swbt_ipc_server_t *server,
                                                               swbt_ipc_connection_t *connection);
swbt_ipc_server_result_t swbt_ipc_server_get_status(const swbt_ipc_server_t *server,
                                                    swbt_ipc_status_t *out_status);
void swbt_ipc_connection_close(swbt_ipc_connection_t *connection);
void swbt_ipc_server_close(swbt_ipc_server_t *server);

#endif
