#ifndef SWBT_DAEMON_IPC_RUNNER_H
#define SWBT_DAEMON_IPC_RUNNER_H

#include <stdbool.h>
#include <stdint.h>

#include "daemon/config.h"
#include "ipc/ipc_server.h"
#include "ipc/ipc_session.h"

#define SWBT_DAEMON_IPC_ENDPOINT_HOST_SIZE 16u

typedef enum {
    SWBT_DAEMON_IPC_RUNNER_OK = 0,
    SWBT_DAEMON_IPC_RUNNER_ERROR_INVALID_ARGUMENT = -1,
    SWBT_DAEMON_IPC_RUNNER_ERROR_SOCKET = -2,
    SWBT_DAEMON_IPC_RUNNER_ERROR_UNSUPPORTED_BIND = -3,
    SWBT_DAEMON_IPC_RUNNER_ERROR_DISCONNECTED = -4,
    SWBT_DAEMON_IPC_RUNNER_ERROR_NOT_RUNNING = -5,
} swbt_daemon_ipc_runner_result_t;

typedef struct {
    const char *host;
    uint16_t port;
    int backlog;
    uint32_t heartbeat_timeout_ms;
} swbt_daemon_ipc_runner_config_t;

typedef struct {
    char host[SWBT_DAEMON_IPC_ENDPOINT_HOST_SIZE];
    uint16_t port;
    bool bound;
} swbt_daemon_ipc_endpoint_t;

typedef struct {
    swbt_ipc_server_t server;
    swbt_ipc_connection_t connection;
    swbt_daemon_ipc_runner_config_t config;
    swbt_daemon_ipc_endpoint_t endpoint;
    bool initialized;
    bool running;
    bool has_connection;
} swbt_daemon_ipc_runner_t;

swbt_daemon_ipc_runner_config_t
swbt_daemon_ipc_runner_config_from_daemon_config(const swbt_daemon_config_t *config);

swbt_daemon_ipc_runner_result_t swbt_daemon_ipc_runner_init(swbt_daemon_ipc_runner_t *runner);

swbt_daemon_ipc_runner_result_t
swbt_daemon_ipc_runner_start(swbt_daemon_ipc_runner_t *runner, swbt_ipc_session_t *session,
                             const swbt_daemon_ipc_runner_config_t *config);

swbt_daemon_ipc_runner_result_t
swbt_daemon_ipc_runner_endpoint(const swbt_daemon_ipc_runner_t *runner,
                                swbt_daemon_ipc_endpoint_t *out_endpoint);

swbt_daemon_ipc_runner_result_t swbt_daemon_ipc_runner_accept(swbt_daemon_ipc_runner_t *runner);

swbt_daemon_ipc_runner_result_t
swbt_daemon_ipc_runner_serve_connection_once(swbt_daemon_ipc_runner_t *runner);

swbt_daemon_ipc_runner_result_t swbt_daemon_ipc_runner_poll_once(swbt_daemon_ipc_runner_t *runner);

swbt_daemon_ipc_runner_result_t
swbt_daemon_ipc_runner_poll_once_at(swbt_daemon_ipc_runner_t *runner, uint64_t now_ms);

void swbt_daemon_ipc_runner_stop(swbt_daemon_ipc_runner_t *runner);

bool swbt_daemon_ipc_runner_is_running(const swbt_daemon_ipc_runner_t *runner);

bool swbt_daemon_ipc_runner_has_connection(const swbt_daemon_ipc_runner_t *runner);

int swbt_daemon_ipc_runner_backend_start(void *context, swbt_ipc_session_t *session);

void swbt_daemon_ipc_runner_backend_stop(void *context);

#endif
