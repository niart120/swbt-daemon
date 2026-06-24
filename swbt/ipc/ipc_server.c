#include "ipc/ipc_server.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET swbt_native_socket_t;
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int swbt_native_socket_t;
#endif

#include "ipc/ipc_adapter.h"

static swbt_native_socket_t swbt_ipc_native_socket(const swbt_ipc_socket_t *socket) {
    return (swbt_native_socket_t)socket->handle;
}

static void swbt_ipc_store_native_socket(swbt_ipc_socket_t *socket,
                                         swbt_native_socket_t native_socket) {
    socket->handle = (uintptr_t)native_socket;
    socket->open = true;
}

static swbt_ipc_server_result_t swbt_ipc_socket_runtime_init(void) {
#ifdef _WIN32
    static bool started = false;
    WSADATA data;

    if (started) {
        return SWBT_IPC_SERVER_OK;
    }
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
    started = true;
#endif
    return SWBT_IPC_SERVER_OK;
}

void swbt_ipc_socket_init(swbt_ipc_socket_t *socket) {
    if (socket == NULL) {
        return;
    }
    socket->handle = 0;
    socket->open = false;
}

void swbt_ipc_socket_close(swbt_ipc_socket_t *socket) {
    if (socket == NULL || !socket->open) {
        return;
    }

#ifdef _WIN32
    (void)closesocket(swbt_ipc_native_socket(socket));
#else
    (void)close(swbt_ipc_native_socket(socket));
#endif
    socket->handle = 0;
    socket->open = false;
}

void swbt_ipc_connection_configure_heartbeat(swbt_ipc_connection_t *connection,
                                             swbt_ipc_heartbeat_config_t config) {
    if (connection == NULL) {
        return;
    }

    connection->last_heartbeat_ms = config.now_ms;
    connection->heartbeat_timeout_ms = config.timeout_ms;
    connection->heartbeat_enabled = config.timeout_ms > 0u;
}

void swbt_ipc_connection_record_heartbeat(swbt_ipc_connection_t *connection, uint64_t now_ms) {
    if (connection == NULL || !connection->heartbeat_enabled) {
        return;
    }

    connection->last_heartbeat_ms = now_ms;
}

static swbt_ipc_server_result_t swbt_ipc_set_reuseaddr(swbt_native_socket_t native_socket) {
    int enabled = 1;
#ifdef _WIN32
    if (setsockopt(native_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&enabled,
                   (int)sizeof(enabled)) != 0) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
#else
    if (setsockopt(native_socket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)) != 0) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
#endif
    return SWBT_IPC_SERVER_OK;
}

static swbt_ipc_server_result_t swbt_ipc_create_tcp_socket(swbt_ipc_socket_t *out_socket) {
    swbt_native_socket_t native_socket;

    if (swbt_ipc_socket_runtime_init() != SWBT_IPC_SERVER_OK) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }

    native_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
    if (native_socket == INVALID_SOCKET) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
#else
    if (native_socket < 0) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
#endif

    swbt_ipc_socket_init(out_socket);
    swbt_ipc_store_native_socket(out_socket, native_socket);
    return SWBT_IPC_SERVER_OK;
}

swbt_ipc_server_result_t swbt_ipc_socket_connect_loopback(swbt_ipc_socket_t *socket,
                                                          uint16_t port) {
    struct sockaddr_in address = {0};

    if (socket == NULL || port == 0) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }
    if (swbt_ipc_create_tcp_socket(socket) != SWBT_IPC_SERVER_OK) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);

    if (connect(swbt_ipc_native_socket(socket), (struct sockaddr *)&address, sizeof(address)) !=
        0) {
        swbt_ipc_socket_close(socket);
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }

    return SWBT_IPC_SERVER_OK;
}

swbt_ipc_server_result_t swbt_ipc_socket_send_all(swbt_ipc_socket_t *socket, const char *data,
                                                  size_t size) {
    size_t sent_total = 0;

    if (socket == NULL || !socket->open || data == NULL) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }

    while (sent_total < size) {
        const size_t remaining = size - sent_total;
#ifdef _WIN32
        const int chunk = remaining > (size_t)INT_MAX ? INT_MAX : (int)remaining;
        const int sent = send(swbt_ipc_native_socket(socket), data + sent_total, chunk, 0);
#else
        const ssize_t sent = send(swbt_ipc_native_socket(socket), data + sent_total, remaining, 0);
#endif
        if (sent <= 0) {
            return SWBT_IPC_SERVER_ERROR_SOCKET;
        }
        sent_total += (size_t)sent;
    }

    return SWBT_IPC_SERVER_OK;
}

swbt_ipc_server_result_t swbt_ipc_socket_receive(swbt_ipc_socket_t *socket, char *buffer,
                                                 size_t buffer_size, size_t *out_received) {
    if (socket == NULL || !socket->open || buffer == NULL || buffer_size == 0 ||
        out_received == NULL) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }

#ifdef _WIN32
    const int capped_size = buffer_size > (size_t)INT_MAX ? INT_MAX : (int)buffer_size;
    const int received = recv(swbt_ipc_native_socket(socket), buffer, capped_size, 0);
#else
    const ssize_t received = recv(swbt_ipc_native_socket(socket), buffer, buffer_size, 0);
#endif
    if (received == 0) {
        *out_received = 0;
        return SWBT_IPC_SERVER_ERROR_DISCONNECTED;
    }
    if (received < 0) {
        *out_received = 0;
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }

    *out_received = (size_t)received;
    return SWBT_IPC_SERVER_OK;
}

swbt_ipc_server_result_t swbt_ipc_socket_can_receive(const swbt_ipc_socket_t *socket,
                                                     bool *out_ready) {
    fd_set read_set;
    struct timeval timeout = {0};
    swbt_native_socket_t native_socket;

    if (socket == NULL || !socket->open || out_ready == NULL) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }

    native_socket = swbt_ipc_native_socket(socket);
    FD_ZERO(&read_set);
    FD_SET(native_socket, &read_set);

#ifdef _WIN32
    const int ready = select(0, &read_set, NULL, NULL, &timeout);
#else
    const int ready = select(native_socket + 1, &read_set, NULL, NULL, &timeout);
#endif
    if (ready < 0) {
        *out_ready = false;
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }

    *out_ready = ready > 0 && FD_ISSET(native_socket, &read_set);
    return SWBT_IPC_SERVER_OK;
}

swbt_ipc_server_result_t swbt_ipc_server_init(swbt_ipc_server_t *server) {
    if (server == NULL) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }

    *server = (swbt_ipc_server_t){0};
    swbt_ipc_socket_init(&server->listen_socket);
    server->default_app = swbt_app_create();
    if (server->default_app == NULL) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
    server->app = server->default_app;
    server->next_client_id = 1;
    server->bound_port = 0;
    server->listening = false;
    return SWBT_IPC_SERVER_OK;
}

swbt_ipc_server_result_t swbt_ipc_server_bind_app(swbt_ipc_server_t *server, swbt_app_t *app) {
    if (server == NULL || app == NULL || server->listening) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }

    server->app = app;
    return SWBT_IPC_SERVER_OK;
}

swbt_ipc_server_result_t swbt_ipc_server_listen(swbt_ipc_server_t *server,
                                                swbt_ipc_server_listen_options_t options) {
    struct sockaddr_in address = {0};
    struct sockaddr_in bound_address = {0};
#ifdef _WIN32
    int bound_address_size = (int)sizeof(bound_address);
#else
    socklen_t bound_address_size = (socklen_t)sizeof(bound_address);
#endif

    if (server == NULL || options.host == NULL || options.backlog <= 0) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }
    if (strcmp(options.host, "127.0.0.1") != 0) {
        return SWBT_IPC_SERVER_ERROR_UNSUPPORTED_BIND;
    }
    if (swbt_ipc_create_tcp_socket(&server->listen_socket) != SWBT_IPC_SERVER_OK) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
    if (swbt_ipc_set_reuseaddr(swbt_ipc_native_socket(&server->listen_socket)) !=
        SWBT_IPC_SERVER_OK) {
        swbt_ipc_socket_close(&server->listen_socket);
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(options.port);

    if (bind(swbt_ipc_native_socket(&server->listen_socket), (struct sockaddr *)&address,
             sizeof(address)) != 0) {
        swbt_ipc_socket_close(&server->listen_socket);
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
    if (listen(swbt_ipc_native_socket(&server->listen_socket), options.backlog) != 0) {
        swbt_ipc_socket_close(&server->listen_socket);
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
    if (getsockname(swbt_ipc_native_socket(&server->listen_socket),
                    (struct sockaddr *)&bound_address, &bound_address_size) != 0) {
        swbt_ipc_socket_close(&server->listen_socket);
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }

    server->bound_port = ntohs(bound_address.sin_port);
    server->listening = true;
    return SWBT_IPC_SERVER_OK;
}

uint16_t swbt_ipc_server_port(const swbt_ipc_server_t *server) {
    if (server == NULL || !server->listening) {
        return 0;
    }
    return server->bound_port;
}

swbt_ipc_server_result_t swbt_ipc_server_has_pending_connection(const swbt_ipc_server_t *server,
                                                                bool *out_pending) {
    if (server == NULL || !server->listening || out_pending == NULL) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }

    return swbt_ipc_socket_can_receive(&server->listen_socket, out_pending);
}

swbt_ipc_server_result_t swbt_ipc_server_accept(swbt_ipc_server_t *server,
                                                swbt_ipc_connection_t *out_connection) {
    swbt_native_socket_t accepted_socket;

    if (server == NULL || out_connection == NULL || !server->listening) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }

    swbt_ipc_socket_init(&out_connection->socket);
    out_connection->client_id = 0;
    out_connection->line_buffer[0] = '\0';
    out_connection->line_length = 0;
    out_connection->last_heartbeat_ms = 0;
    out_connection->heartbeat_timeout_ms = 0;
    out_connection->heartbeat_enabled = false;
    out_connection->open = false;

    accepted_socket = accept(swbt_ipc_native_socket(&server->listen_socket), NULL, NULL);
#ifdef _WIN32
    if (accepted_socket == INVALID_SOCKET) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
#else
    if (accepted_socket < 0) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
#endif

    swbt_ipc_store_native_socket(&out_connection->socket, accepted_socket);
    out_connection->client_id = server->next_client_id;
    out_connection->open = true;
    ++server->next_client_id;
    return SWBT_IPC_SERVER_OK;
}

swbt_ipc_server_result_t
swbt_ipc_connection_has_pending_data(const swbt_ipc_connection_t *connection, bool *out_pending) {
    if (connection == NULL || !connection->open || out_pending == NULL) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }

    return swbt_ipc_socket_can_receive(&connection->socket, out_pending);
}

static swbt_ipc_server_result_t swbt_ipc_read_line(swbt_ipc_connection_t *connection, char *line,
                                                   size_t line_size, size_t *out_length,
                                                   bool *out_complete) {
    size_t length = 0;

    if (connection == NULL || !connection->open || line == NULL || line_size == 0 ||
        out_length == NULL || out_complete == NULL) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }

    *out_length = 0;
    *out_complete = false;
    length = connection->line_length;

    while (true) {
        bool ready = false;
        char byte = '\0';
        size_t received = 0;
        swbt_ipc_server_result_t result = swbt_ipc_socket_can_receive(&connection->socket, &ready);
        if (result != SWBT_IPC_SERVER_OK) {
            return result;
        }
        if (!ready) {
            connection->line_length = length;
            connection->line_buffer[length] = '\0';
            return SWBT_IPC_SERVER_OK;
        }

        result = swbt_ipc_socket_receive(&connection->socket, &byte, 1, &received);
        if (result != SWBT_IPC_SERVER_OK) {
            *out_length = 0;
            return result;
        }
        if (received != 1) {
            *out_length = 0;
            return SWBT_IPC_SERVER_ERROR_SOCKET;
        }
        if (length + 1 >= line_size) {
            *out_length = 0;
            connection->line_length = 0;
            connection->line_buffer[0] = '\0';
            return SWBT_IPC_SERVER_ERROR_MESSAGE_TOO_LONG;
        }

        connection->line_buffer[length] = byte;
        ++length;
        if (byte == '\n') {
            memcpy(line, connection->line_buffer, length);
            line[length] = '\0';
            *out_length = length;
            *out_complete = true;
            connection->line_length = 0;
            connection->line_buffer[0] = '\0';
            return SWBT_IPC_SERVER_OK;
        }
    }
}

static swbt_ipc_server_result_t
swbt_ipc_server_serve_connection_once_internal(swbt_ipc_server_t *server,
                                               swbt_ipc_connection_t *connection,
                                               bool update_heartbeat, uint64_t now_ms) {
    char line[SWBT_IPC_JSON_LINE_MAX + 1u];
    char response[SWBT_IPC_JSON_RESPONSE_MAX];
    size_t line_length = 0;
    bool line_complete = false;
    swbt_ipc_server_result_t read_result;
    swbt_ipc_json_result_t json_result;

    if (server == NULL || connection == NULL || !connection->open) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }

    read_result = swbt_ipc_read_line(connection, line, sizeof(line), &line_length, &line_complete);
    if (read_result == SWBT_IPC_SERVER_ERROR_DISCONNECTED ||
        read_result == SWBT_IPC_SERVER_ERROR_SOCKET) {
        (void)swbt_ipc_adapter_handle_disconnect(server->app, connection->client_id);
        return SWBT_IPC_SERVER_ERROR_DISCONNECTED;
    }
    if (read_result == SWBT_IPC_SERVER_ERROR_MESSAGE_TOO_LONG) {
        (void)swbt_ipc_adapter_handle_disconnect(server->app, connection->client_id);
        swbt_ipc_connection_close(connection);
        return SWBT_IPC_SERVER_ERROR_MESSAGE_TOO_LONG;
    }
    if (read_result != SWBT_IPC_SERVER_OK) {
        return read_result;
    }
    if (!line_complete) {
        return SWBT_IPC_SERVER_OK;
    }

    (void)line_length;
    if (update_heartbeat) {
        swbt_ipc_connection_record_heartbeat(connection, now_ms);
    }
    json_result = swbt_ipc_adapter_handle_line(server->app, connection->client_id, line, response,
                                               sizeof(response));
    if (json_result != SWBT_IPC_JSON_OK) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
    if (response[0] == '\0') {
        return SWBT_IPC_SERVER_OK;
    }

    return swbt_ipc_socket_send_all(&connection->socket, response, strlen(response));
}

swbt_ipc_server_result_t swbt_ipc_server_serve_connection_once(swbt_ipc_server_t *server,
                                                               swbt_ipc_connection_t *connection) {
    return swbt_ipc_server_serve_connection_once_internal(server, connection, false, 0);
}

swbt_ipc_server_result_t swbt_ipc_server_serve_connection_once_at(swbt_ipc_server_t *server,
                                                                  swbt_ipc_connection_t *connection,
                                                                  uint64_t now_ms) {
    return swbt_ipc_server_serve_connection_once_internal(server, connection, true, now_ms);
}

swbt_ipc_server_result_t swbt_ipc_server_check_heartbeat(swbt_ipc_server_t *server,
                                                         swbt_ipc_connection_t *connection,
                                                         uint64_t now_ms) {
    uint64_t elapsed_ms = 0;

    if (server == NULL || connection == NULL) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }
    if (!connection->heartbeat_enabled) {
        return SWBT_IPC_SERVER_OK;
    }
    if (now_ms < connection->last_heartbeat_ms) {
        return SWBT_IPC_SERVER_OK;
    }

    elapsed_ms = now_ms - connection->last_heartbeat_ms;
    if (elapsed_ms < connection->heartbeat_timeout_ms) {
        return SWBT_IPC_SERVER_OK;
    }

    (void)swbt_ipc_adapter_handle_heartbeat_timeout(server->app, connection->client_id);
    return SWBT_IPC_SERVER_ERROR_HEARTBEAT_TIMEOUT;
}

swbt_ipc_server_result_t swbt_ipc_server_get_status(const swbt_ipc_server_t *server,
                                                    swbt_ipc_status_t *out_status) {
    if (server == NULL || out_status == NULL) {
        return SWBT_IPC_SERVER_ERROR_INVALID_ARGUMENT;
    }
    if (swbt_ipc_adapter_get_status(server->app, out_status) != SWBT_IPC_OK) {
        return SWBT_IPC_SERVER_ERROR_SOCKET;
    }
    return SWBT_IPC_SERVER_OK;
}

void swbt_ipc_connection_close(swbt_ipc_connection_t *connection) {
    if (connection == NULL) {
        return;
    }
    swbt_ipc_socket_close(&connection->socket);
    connection->client_id = 0;
    connection->line_buffer[0] = '\0';
    connection->line_length = 0;
    connection->last_heartbeat_ms = 0;
    connection->heartbeat_timeout_ms = 0;
    connection->heartbeat_enabled = false;
    connection->open = false;
}

void swbt_ipc_server_close(swbt_ipc_server_t *server) {
    if (server == NULL) {
        return;
    }
    swbt_ipc_socket_close(&server->listen_socket);
    if (server->default_app != NULL) {
        swbt_app_destroy(server->default_app);
    }
    server->default_app = NULL;
    server->app = NULL;
    server->bound_port = 0;
    server->listening = false;
}
