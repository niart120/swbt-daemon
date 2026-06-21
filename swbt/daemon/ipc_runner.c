#include "daemon/ipc_runner.h"

#include <stddef.h>

static swbt_daemon_ipc_runner_result_t
swbt_daemon_ipc_runner_map_server_result(swbt_ipc_server_result_t result) {
    switch (result) {
    case SWBT_IPC_SERVER_OK:
        return SWBT_DAEMON_IPC_RUNNER_OK;
    case SWBT_IPC_SERVER_ERROR_UNSUPPORTED_BIND:
        return SWBT_DAEMON_IPC_RUNNER_ERROR_UNSUPPORTED_BIND;
    case SWBT_IPC_SERVER_ERROR_DISCONNECTED:
        return SWBT_DAEMON_IPC_RUNNER_ERROR_DISCONNECTED;
    case SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT:
        return SWBT_DAEMON_IPC_RUNNER_ERROR_INVALID_ARGUMENT;
    case SWBT_IPC_SERVER_ERROR_SOCKET:
    case SWBT_IPC_SERVER_ERROR_MESSAGE_TOO_LONG:
    case SWBT_IPC_SERVER_ERROR_HEARTBEAT_TIMEOUT:
        return SWBT_DAEMON_IPC_RUNNER_ERROR_SOCKET;
    }

    return SWBT_DAEMON_IPC_RUNNER_ERROR_SOCKET;
}

static void swbt_daemon_ipc_runner_clear_endpoint(swbt_daemon_ipc_runner_t *runner) {
    runner->endpoint.host[0] = '\0';
    runner->endpoint.port = 0u;
    runner->endpoint.bound = false;
}

static void swbt_daemon_ipc_runner_copy_host(swbt_daemon_ipc_runner_t *runner, const char *host) {
    size_t index = 0;

    for (; index + 1u < SWBT_DAEMON_IPC_ENDPOINT_HOST_SIZE && host[index] != '\0'; ++index) {
        runner->endpoint.host[index] = host[index];
    }
    runner->endpoint.host[index] = '\0';
}

swbt_daemon_ipc_runner_config_t
swbt_daemon_ipc_runner_config_from_daemon_config(const swbt_daemon_config_t *config) {
    if (config == NULL) {
        return (swbt_daemon_ipc_runner_config_t){0};
    }

    return (swbt_daemon_ipc_runner_config_t){
        .host = config->ipc_host,
        .port = config->ipc_port,
        .backlog = config->ipc_backlog,
        .heartbeat_timeout_ms = config->ipc_heartbeat_timeout_ms,
    };
}

swbt_daemon_ipc_runner_result_t swbt_daemon_ipc_runner_init(swbt_daemon_ipc_runner_t *runner) {
    if (runner == NULL) {
        return SWBT_DAEMON_IPC_RUNNER_ERROR_INVALID_ARGUMENT;
    }

    *runner = (swbt_daemon_ipc_runner_t){0};
    if (swbt_ipc_server_init(&runner->server) != SWBT_IPC_SERVER_OK) {
        return SWBT_DAEMON_IPC_RUNNER_ERROR_SOCKET;
    }
    swbt_ipc_socket_init(&runner->connection.socket);
    runner->config = (swbt_daemon_ipc_runner_config_t){
        .host = SWBT_DAEMON_DEFAULT_IPC_HOST,
        .port = SWBT_DAEMON_DEFAULT_IPC_PORT,
        .backlog = SWBT_DAEMON_DEFAULT_IPC_BACKLOG,
        .heartbeat_timeout_ms = SWBT_DAEMON_DEFAULT_IPC_HEARTBEAT_TIMEOUT_MS,
    };
    swbt_daemon_ipc_runner_clear_endpoint(runner);
    runner->initialized = true;
    return SWBT_DAEMON_IPC_RUNNER_OK;
}

swbt_daemon_ipc_runner_result_t
swbt_daemon_ipc_runner_start(swbt_daemon_ipc_runner_t *runner, swbt_ipc_session_t *session,
                             const swbt_daemon_ipc_runner_config_t *config) {
    swbt_ipc_server_result_t result;

    if (runner == NULL || session == NULL || config == NULL || config->host == NULL ||
        config->backlog <= 0 || !runner->initialized) {
        return SWBT_DAEMON_IPC_RUNNER_ERROR_INVALID_ARGUMENT;
    }
    if (runner->running) {
        return SWBT_DAEMON_IPC_RUNNER_OK;
    }

    result = swbt_ipc_server_bind_session(&runner->server, session);
    if (result != SWBT_IPC_SERVER_OK) {
        return swbt_daemon_ipc_runner_map_server_result(result);
    }
    result = swbt_ipc_server_listen(&runner->server, config->host, config->port, config->backlog);
    if (result != SWBT_IPC_SERVER_OK) {
        swbt_ipc_server_close(&runner->server);
        return swbt_daemon_ipc_runner_map_server_result(result);
    }

    runner->config = *config;
    swbt_daemon_ipc_runner_copy_host(runner, config->host);
    runner->endpoint.port = swbt_ipc_server_port(&runner->server);
    runner->endpoint.bound = runner->endpoint.port != 0u;
    runner->running = true;
    runner->has_connection = false;
    return SWBT_DAEMON_IPC_RUNNER_OK;
}

swbt_daemon_ipc_runner_result_t
swbt_daemon_ipc_runner_endpoint(const swbt_daemon_ipc_runner_t *runner,
                                swbt_daemon_ipc_endpoint_t *out_endpoint) {
    if (runner == NULL || out_endpoint == NULL || !runner->initialized) {
        return SWBT_DAEMON_IPC_RUNNER_ERROR_INVALID_ARGUMENT;
    }

    *out_endpoint = runner->endpoint;
    return SWBT_DAEMON_IPC_RUNNER_OK;
}

static swbt_daemon_ipc_runner_result_t
swbt_daemon_ipc_runner_accept_at(swbt_daemon_ipc_runner_t *runner, uint64_t now_ms) {
    swbt_ipc_server_result_t result;

    if (runner == NULL || !runner->running) {
        return SWBT_DAEMON_IPC_RUNNER_ERROR_NOT_RUNNING;
    }
    if (runner->has_connection) {
        return SWBT_DAEMON_IPC_RUNNER_ERROR_INVALID_ARGUMENT;
    }

    result = swbt_ipc_server_accept(&runner->server, &runner->connection);
    if (result != SWBT_IPC_SERVER_OK) {
        return swbt_daemon_ipc_runner_map_server_result(result);
    }

    swbt_ipc_connection_configure_heartbeat(&runner->connection,
                                            (swbt_ipc_heartbeat_config_t){
                                                .now_ms = now_ms,
                                                .timeout_ms = runner->config.heartbeat_timeout_ms,
                                            });
    runner->has_connection = true;
    return SWBT_DAEMON_IPC_RUNNER_OK;
}

swbt_daemon_ipc_runner_result_t swbt_daemon_ipc_runner_accept(swbt_daemon_ipc_runner_t *runner) {
    return swbt_daemon_ipc_runner_accept_at(runner, 0u);
}

swbt_daemon_ipc_runner_result_t
swbt_daemon_ipc_runner_serve_connection_once(swbt_daemon_ipc_runner_t *runner) {
    swbt_ipc_server_result_t result;

    if (runner == NULL || !runner->running) {
        return SWBT_DAEMON_IPC_RUNNER_ERROR_NOT_RUNNING;
    }
    if (!runner->has_connection) {
        return SWBT_DAEMON_IPC_RUNNER_ERROR_INVALID_ARGUMENT;
    }

    result = swbt_ipc_server_serve_connection_once(&runner->server, &runner->connection);
    if (result == SWBT_IPC_SERVER_ERROR_DISCONNECTED) {
        swbt_ipc_connection_close(&runner->connection);
        runner->has_connection = false;
    }
    return swbt_daemon_ipc_runner_map_server_result(result);
}

static swbt_daemon_ipc_runner_result_t
swbt_daemon_ipc_runner_serve_connection_once_at(swbt_daemon_ipc_runner_t *runner,
                                                uint64_t now_ms) {
    swbt_ipc_server_result_t result;

    if (runner == NULL || !runner->running) {
        return SWBT_DAEMON_IPC_RUNNER_ERROR_NOT_RUNNING;
    }
    if (!runner->has_connection) {
        return SWBT_DAEMON_IPC_RUNNER_ERROR_INVALID_ARGUMENT;
    }

    result = swbt_ipc_server_serve_connection_once_at(&runner->server, &runner->connection,
                                                      now_ms);
    if (result == SWBT_IPC_SERVER_ERROR_DISCONNECTED) {
        swbt_ipc_connection_close(&runner->connection);
        runner->has_connection = false;
    }
    return swbt_daemon_ipc_runner_map_server_result(result);
}

static swbt_daemon_ipc_runner_result_t
swbt_daemon_ipc_runner_check_heartbeat(swbt_daemon_ipc_runner_t *runner, uint64_t now_ms) {
    const swbt_ipc_server_result_t result =
        swbt_ipc_server_check_heartbeat(&runner->server, &runner->connection, now_ms);
    if (result == SWBT_IPC_SERVER_ERROR_HEARTBEAT_TIMEOUT) {
        swbt_ipc_connection_close(&runner->connection);
        runner->has_connection = false;
        return SWBT_DAEMON_IPC_RUNNER_OK;
    }
    return swbt_daemon_ipc_runner_map_server_result(result);
}

swbt_daemon_ipc_runner_result_t
swbt_daemon_ipc_runner_poll_once_at(swbt_daemon_ipc_runner_t *runner, uint64_t now_ms) {
    bool pending = false;
    swbt_ipc_server_result_t result;

    if (runner == NULL || !runner->running) {
        return SWBT_DAEMON_IPC_RUNNER_ERROR_NOT_RUNNING;
    }

    if (!runner->has_connection) {
        result = swbt_ipc_server_has_pending_connection(&runner->server, &pending);
        if (result != SWBT_IPC_SERVER_OK) {
            return swbt_daemon_ipc_runner_map_server_result(result);
        }
        if (!pending) {
            return SWBT_DAEMON_IPC_RUNNER_OK;
        }
        return swbt_daemon_ipc_runner_accept_at(runner, now_ms);
    }

    result = swbt_ipc_connection_has_pending_data(&runner->connection, &pending);
    if (result != SWBT_IPC_SERVER_OK) {
        return swbt_daemon_ipc_runner_map_server_result(result);
    }
    if (!pending) {
        return swbt_daemon_ipc_runner_check_heartbeat(runner, now_ms);
    }

    return swbt_daemon_ipc_runner_serve_connection_once_at(runner, now_ms);
}

swbt_daemon_ipc_runner_result_t swbt_daemon_ipc_runner_poll_once(swbt_daemon_ipc_runner_t *runner) {
    return swbt_daemon_ipc_runner_poll_once_at(runner, 0u);
}

void swbt_daemon_ipc_runner_stop(swbt_daemon_ipc_runner_t *runner) {
    if (runner == NULL || !runner->initialized) {
        return;
    }
    if (!runner->running && !runner->has_connection) {
        swbt_daemon_ipc_runner_clear_endpoint(runner);
        return;
    }

    if (runner->has_connection) {
        (void)swbt_ipc_disconnect(runner->server.session, runner->connection.client_id);
        swbt_ipc_connection_close(&runner->connection);
        runner->has_connection = false;
    } else if (runner->server.session != NULL) {
        (void)swbt_ipc_clear_owner(runner->server.session);
    }

    swbt_ipc_server_close(&runner->server);
    swbt_daemon_ipc_runner_clear_endpoint(runner);
    runner->running = false;
}

bool swbt_daemon_ipc_runner_is_running(const swbt_daemon_ipc_runner_t *runner) {
    return runner != NULL && runner->running;
}

bool swbt_daemon_ipc_runner_has_connection(const swbt_daemon_ipc_runner_t *runner) {
    return runner != NULL && runner->has_connection;
}

int swbt_daemon_ipc_runner_backend_start(void *context, swbt_ipc_session_t *session) {
    swbt_daemon_ipc_runner_t *runner = context;
    if (runner == NULL) {
        return -1;
    }

    return swbt_daemon_ipc_runner_start(runner, session, &runner->config) ==
                   SWBT_DAEMON_IPC_RUNNER_OK
               ? 0
               : -1;
}

void swbt_daemon_ipc_runner_backend_stop(void *context) {
    swbt_daemon_ipc_runner_stop((swbt_daemon_ipc_runner_t *)context);
}
