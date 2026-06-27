#ifndef SWBT_IPC_SERVER_H
#define SWBT_IPC_SERVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "application/app.h"
#include "control/control.h"
#include "ipc/ipc_json.h"
#include "ipc/ipc_status.h"

typedef enum {
    SWBT_IPC_SERVER_OK = 0,
    SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT = -1,
    SWBT_IPC_SERVER_ERROR_SOCKET = -2,
    SWBT_IPC_SERVER_ERROR_UNSUPPORTED_BIND = -3,
    SWBT_IPC_SERVER_ERROR_DISCONNECTED = -4,
    SWBT_IPC_SERVER_ERROR_MESSAGE_TOO_LONG = -5,
    SWBT_IPC_SERVER_ERROR_HEARTBEAT_TIMEOUT = -6,
} swbt_ipc_server_result_t;

typedef struct {
    uintptr_t handle;
    bool open;
} swbt_ipc_socket_t;

typedef struct {
    swbt_ipc_socket_t socket;
    uint32_t client_id;
    char line_buffer[SWBT_IPC_JSON_LINE_MAX + 1u];
    size_t line_length;
    uint64_t last_heartbeat_ms;
    uint64_t heartbeat_timeout_ms;
    bool heartbeat_enabled;
    bool open;
} swbt_ipc_connection_t;

typedef struct {
    uint64_t now_ms;
    uint64_t timeout_ms;
} swbt_ipc_heartbeat_config_t;

typedef struct {
    const char *host;
    uint16_t port;
    int backlog;
} swbt_ipc_server_listen_options_t;

typedef struct {
    swbt_ipc_socket_t listen_socket;
    swbt_app_t *default_app;
    swbt_control_t default_control;
    swbt_control_t *control;
    uint32_t next_client_id;
    uint16_t bound_port;
    bool listening;
} swbt_ipc_server_t;

void swbt_ipc_socket_init(swbt_ipc_socket_t *socket);
void swbt_ipc_socket_close(swbt_ipc_socket_t *socket);
void swbt_ipc_connection_configure_heartbeat(swbt_ipc_connection_t *connection,
                                             swbt_ipc_heartbeat_config_t config);
void swbt_ipc_connection_record_heartbeat(swbt_ipc_connection_t *connection, uint64_t now_ms);

swbt_ipc_server_result_t swbt_ipc_socket_connect_loopback(swbt_ipc_socket_t *socket, uint16_t port);
swbt_ipc_server_result_t swbt_ipc_socket_send_all(swbt_ipc_socket_t *socket, const char *data,
                                                  size_t size);
swbt_ipc_server_result_t swbt_ipc_socket_receive(swbt_ipc_socket_t *socket, char *buffer,
                                                 size_t buffer_size, size_t *out_received);
swbt_ipc_server_result_t swbt_ipc_socket_can_receive(const swbt_ipc_socket_t *socket,
                                                     bool *out_ready);

swbt_ipc_server_result_t swbt_ipc_server_init(swbt_ipc_server_t *server);
swbt_ipc_server_result_t swbt_ipc_server_bind_control(swbt_ipc_server_t *server,
                                                      swbt_control_t *control);
swbt_ipc_server_result_t swbt_ipc_server_listen(swbt_ipc_server_t *server,
                                                swbt_ipc_server_listen_options_t options);
uint16_t swbt_ipc_server_port(const swbt_ipc_server_t *server);
swbt_ipc_server_result_t swbt_ipc_server_has_pending_connection(const swbt_ipc_server_t *server,
                                                                bool *out_pending);
swbt_ipc_server_result_t swbt_ipc_server_accept(swbt_ipc_server_t *server,
                                                swbt_ipc_connection_t *out_connection);
swbt_ipc_server_result_t
swbt_ipc_connection_has_pending_data(const swbt_ipc_connection_t *connection, bool *out_pending);
swbt_ipc_server_result_t swbt_ipc_server_serve_connection_once(swbt_ipc_server_t *server,
                                                               swbt_ipc_connection_t *connection);
swbt_ipc_server_result_t swbt_ipc_server_serve_connection_once_at(swbt_ipc_server_t *server,
                                                                  swbt_ipc_connection_t *connection,
                                                                  uint64_t now_ms);
swbt_ipc_server_result_t swbt_ipc_server_check_heartbeat(swbt_ipc_server_t *server,
                                                         swbt_ipc_connection_t *connection,
                                                         uint64_t now_ms);
swbt_ipc_server_result_t swbt_ipc_server_get_status(const swbt_ipc_server_t *server,
                                                    swbt_ipc_status_t *out_status);
void swbt_ipc_connection_close(swbt_ipc_connection_t *connection);
void swbt_ipc_server_close(swbt_ipc_server_t *server);

#endif
