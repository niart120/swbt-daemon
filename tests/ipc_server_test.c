#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ipc/ipc_server.h"
#include "switch/switch_controller_state.h"

static int expect_contains(const char *text, const char *needle) {
    return strstr(text, needle) != NULL ? 0 : 1;
}

static void clear_buffer(char *buffer, size_t size) {
    for (size_t index = 0; index < size; ++index) {
        buffer[index] = '\0';
    }
}

static int send_line(swbt_ipc_socket_t *socket, const char *line) {
    return swbt_ipc_socket_send_all(socket, line, strlen(line)) == SWBT_IPC_SERVER_OK ? 0 : 1;
}

static int receive_response(swbt_ipc_socket_t *socket, char *buffer, size_t buffer_size) {
    size_t received = 0;
    clear_buffer(buffer, buffer_size);
    if (swbt_ipc_socket_receive(socket, buffer, buffer_size - 1u, &received) !=
        SWBT_IPC_SERVER_OK) {
        return 1;
    }
    buffer[received] = '\0';
    return 0;
}

static int oversized_line_closes_owner_connection(void) {
    swbt_ipc_server_t server;
    swbt_ipc_connection_t connection;
    swbt_ipc_socket_t client;
    swbt_ipc_status_t status;
    uint16_t port = 0;
    char response[SWBT_IPC_JSON_RESPONSE_MAX];
    char oversized_line[SWBT_IPC_JSON_LINE_MAX + 2u];

    if (swbt_ipc_server_init(&server) != SWBT_IPC_SERVER_OK) {
        return 1;
    }
    if (swbt_ipc_server_listen(&server, (swbt_ipc_server_listen_options_t){
                                            .host = "127.0.0.1",
                                            .port = 0,
                                            .backlog = 1,
                                        }) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_server_close(&server);
        return 2;
    }
    port = swbt_ipc_server_port(&server);

    swbt_ipc_socket_init(&client);
    if (swbt_ipc_socket_connect_loopback(&client, port) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_server_close(&server);
        return 3;
    }
    if (swbt_ipc_server_accept(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 4;
    }

    if (send_line(
            &client,
            "{\"v\":1,\"type\":\"acquire\",\"mode\":\"exclusive\",\"request_id\":\"a1\"}\n") != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 5;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 6;
    }
    if (receive_response(&client, response, sizeof(response)) != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 7;
    }
    if (expect_contains(response, "\"type\":\"acquired\"")) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 8;
    }

    for (size_t index = 0; index < sizeof(oversized_line) - 1u; ++index) {
        oversized_line[index] = 'x';
    }
    oversized_line[sizeof(oversized_line) - 1u] = '\0';
    if (swbt_ipc_socket_send_all(&client, oversized_line, strlen(oversized_line)) !=
        SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 9;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) !=
        SWBT_IPC_SERVER_ERROR_MESSAGE_TOO_LONG) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 10;
    }
    if (connection.open) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 11;
    }
    if (swbt_ipc_server_get_status(&server, &status) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 12;
    }
    if (status.has_owner || status.state.buttons != 0u || status.state.lx != 2048u) {
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 13;
    }

    swbt_ipc_socket_close(&client);
    swbt_ipc_server_close(&server);
    return 0;
}

static int fragmented_valid_command_waits_for_newline(void) {
    swbt_ipc_server_t server;
    swbt_ipc_connection_t connection;
    swbt_ipc_socket_t client;
    uint16_t port = 0;
    bool response_ready = true;
    const char *first_fragment = "{\"v\":1,\"type\":\"hello\"";
    char response[SWBT_IPC_JSON_RESPONSE_MAX];

    if (swbt_ipc_server_init(&server) != SWBT_IPC_SERVER_OK) {
        return 1;
    }
    if (swbt_ipc_server_listen(&server, (swbt_ipc_server_listen_options_t){
                                            .host = "127.0.0.1",
                                            .port = 0,
                                            .backlog = 1,
                                        }) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_server_close(&server);
        return 2;
    }
    port = swbt_ipc_server_port(&server);

    swbt_ipc_socket_init(&client);
    if (swbt_ipc_socket_connect_loopback(&client, port) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_server_close(&server);
        return 3;
    }
    if (swbt_ipc_server_accept(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 4;
    }

    if (swbt_ipc_socket_send_all(&client, first_fragment, strlen(first_fragment)) !=
        SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 5;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 6;
    }
    if (swbt_ipc_socket_can_receive(&client, &response_ready) != SWBT_IPC_SERVER_OK ||
        response_ready) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 7;
    }

    if (send_line(&client, ",\"request_id\":\"h1\"}\n") != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 8;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 9;
    }
    if (receive_response(&client, response, sizeof(response)) != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 10;
    }
    if (expect_contains(response, "\"type\":\"hello_ok\"") ||
        expect_contains(response, "\"request_id\":\"h1\"")) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 11;
    }

    swbt_ipc_connection_close(&connection);
    swbt_ipc_socket_close(&client);
    swbt_ipc_server_close(&server);
    return 0;
}

static int malformed_line_recovers_next_valid_command(void) {
    swbt_ipc_server_t server;
    swbt_ipc_connection_t connection;
    swbt_ipc_socket_t client;
    uint16_t port = 0;
    char response[SWBT_IPC_JSON_RESPONSE_MAX];

    if (swbt_ipc_server_init(&server) != SWBT_IPC_SERVER_OK) {
        return 1;
    }
    if (swbt_ipc_server_listen(&server, (swbt_ipc_server_listen_options_t){
                                            .host = "127.0.0.1",
                                            .port = 0,
                                            .backlog = 1,
                                        }) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_server_close(&server);
        return 2;
    }
    port = swbt_ipc_server_port(&server);

    swbt_ipc_socket_init(&client);
    if (swbt_ipc_socket_connect_loopback(&client, port) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_server_close(&server);
        return 3;
    }
    if (swbt_ipc_server_accept(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 4;
    }

    if (send_line(&client, "{\"v\":1,\"type\":\"hello\",}\n") != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 5;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 6;
    }
    if (receive_response(&client, response, sizeof(response)) != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 7;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"invalid_json\"") || !connection.open) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 8;
    }

    if (send_line(&client, "{\"v\":1,\"type\":\"hello\",\"request_id\":\"h1\"}\n") != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 9;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 10;
    }
    if (receive_response(&client, response, sizeof(response)) != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 11;
    }
    if (expect_contains(response, "\"type\":\"hello_ok\"") ||
        expect_contains(response, "\"request_id\":\"h1\"")) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 12;
    }

    swbt_ipc_connection_close(&connection);
    swbt_ipc_socket_close(&client);
    swbt_ipc_server_close(&server);
    return 0;
}

static int partial_line_does_not_block_heartbeat_timeout(void) {
    swbt_ipc_server_t server;
    swbt_ipc_connection_t connection;
    swbt_ipc_socket_t client;
    swbt_ipc_status_t status;
    uint16_t port = 0;
    bool response_ready = true;
    const char *first_fragment = "{\"v\":1,\"type\":\"get_status\"";
    char response[SWBT_IPC_JSON_RESPONSE_MAX];

    if (swbt_ipc_server_init(&server) != SWBT_IPC_SERVER_OK) {
        return 1;
    }
    if (swbt_ipc_server_listen(&server, (swbt_ipc_server_listen_options_t){
                                            .host = "127.0.0.1",
                                            .port = 0,
                                            .backlog = 1,
                                        }) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_server_close(&server);
        return 2;
    }
    port = swbt_ipc_server_port(&server);

    swbt_ipc_socket_init(&client);
    if (swbt_ipc_socket_connect_loopback(&client, port) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_server_close(&server);
        return 3;
    }
    if (swbt_ipc_server_accept(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 4;
    }

    if (send_line(
            &client,
            "{\"v\":1,\"type\":\"acquire\",\"mode\":\"exclusive\",\"request_id\":\"a1\"}\n") != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 5;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK ||
        receive_response(&client, response, sizeof(response)) != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 6;
    }
    if (send_line(&client, "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"00000001\",\"seq\":5,"
                           "\"request_id\":\"s1\",\"state\":{\"buttons\":8,\"lx\":1234,\"ly\":2048,"
                           "\"rx\":2048,\"ry\":2048,\"accel_x\":0,\"accel_y\":0,\"accel_z\":0,"
                           "\"gyro_x\":0,\"gyro_y\":0,\"gyro_z\":0}}\n") != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 7;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK ||
        receive_response(&client, response, sizeof(response)) != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 8;
    }

    swbt_ipc_connection_configure_heartbeat(&connection, (swbt_ipc_heartbeat_config_t){
                                                             .now_ms = 1000u,
                                                             .timeout_ms = 100u,
                                                         });
    if (swbt_ipc_socket_send_all(&client, first_fragment, strlen(first_fragment)) !=
        SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 9;
    }
    if (swbt_ipc_server_serve_connection_once_at(&server, &connection, 1050u) !=
        SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 10;
    }
    if (swbt_ipc_socket_can_receive(&client, &response_ready) != SWBT_IPC_SERVER_OK ||
        response_ready) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 11;
    }
    if (swbt_ipc_server_check_heartbeat(&server, &connection, 1099u) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 12;
    }
    if (swbt_ipc_server_check_heartbeat(&server, &connection, 1100u) !=
        SWBT_IPC_SERVER_ERROR_HEARTBEAT_TIMEOUT) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 13;
    }
    if (swbt_ipc_server_get_status(&server, &status) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 14;
    }
    if (status.has_owner || status.state.buttons != 0u || status.state.lx != 2048u) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 15;
    }

    swbt_ipc_connection_close(&connection);
    swbt_ipc_socket_close(&client);
    swbt_ipc_server_close(&server);
    return 0;
}

int main(void) {
    swbt_ipc_server_t server;
    swbt_ipc_server_t rejected_server;
    swbt_ipc_connection_t connection;
    swbt_ipc_socket_t client;
    swbt_ipc_status_t status;
    uint16_t port = 0;
    char response[SWBT_IPC_JSON_RESPONSE_MAX];

    if (swbt_ipc_server_init(&rejected_server) != SWBT_IPC_SERVER_OK) {
        return 1;
    }
    if (swbt_ipc_server_listen(&rejected_server, (swbt_ipc_server_listen_options_t){
                                                     .host = "0.0.0.0",
                                                     .port = 0,
                                                     .backlog = 1,
                                                 }) != SWBT_IPC_SERVER_ERROR_UNSUPPORTED_BIND) {
        return 2;
    }
    swbt_ipc_server_close(&rejected_server);

    if (swbt_ipc_server_init(&server) != SWBT_IPC_SERVER_OK) {
        return 3;
    }
    if (swbt_ipc_server_listen(&server, (swbt_ipc_server_listen_options_t){
                                            .host = "127.0.0.1",
                                            .port = 0,
                                            .backlog = 1,
                                        }) != SWBT_IPC_SERVER_OK) {
        return 4;
    }
    port = swbt_ipc_server_port(&server);
    if (port == 0) {
        return 5;
    }

    swbt_ipc_socket_init(&client);
    if (swbt_ipc_socket_connect_loopback(&client, port) != SWBT_IPC_SERVER_OK) {
        return 6;
    }
    if (swbt_ipc_server_accept(&server, &connection) != SWBT_IPC_SERVER_OK) {
        return 7;
    }
    if (connection.client_id != 1u) {
        return 8;
    }

    if (oversized_line_closes_owner_connection() != 0) {
        return 38;
    }
    if (fragmented_valid_command_waits_for_newline() != 0) {
        return 39;
    }
    if (malformed_line_recovers_next_valid_command() != 0) {
        return 40;
    }
    if (partial_line_does_not_block_heartbeat_timeout() != 0) {
        return 41;
    }

    if (send_line(&client, "{\"v\":1,\"type\":\"hello\",\"request_id\":\"h1\"}\n") != 0) {
        return 9;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        return 10;
    }
    if (receive_response(&client, response, sizeof(response)) != 0) {
        return 11;
    }
    if (expect_contains(response, "\"type\":\"hello_ok\"") ||
        expect_contains(response, "\"client_id\":\"00000001\"")) {
        return 12;
    }

    if (send_line(
            &client,
            "{\"v\":1,\"type\":\"acquire\",\"mode\":\"exclusive\",\"request_id\":\"a1\"}\n") != 0) {
        return 13;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        return 14;
    }
    if (receive_response(&client, response, sizeof(response)) != 0) {
        return 15;
    }
    if (expect_contains(response, "\"type\":\"acquired\"") ||
        expect_contains(response, "\"owner_id\":\"00000001\"")) {
        return 16;
    }

    if (send_line(&client, "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"00000001\",\"seq\":5,"
                           "\"request_id\":\"s1\",\"state\":{\"buttons\":8,\"lx\":1234,\"ly\":2048,"
                           "\"rx\":2048,\"ry\":2048,\"accel_x\":0,\"accel_y\":0,\"accel_z\":0,"
                           "\"gyro_x\":0,\"gyro_y\":0,\"gyro_z\":0}}\n") != 0) {
        return 17;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        return 18;
    }
    if (receive_response(&client, response, sizeof(response)) != 0) {
        return 19;
    }
    if (expect_contains(response, "\"type\":\"state_accepted\"") ||
        expect_contains(response, "\"seq\":5")) {
        return 20;
    }

    if (send_line(&client, "{\"v\":1,\"type\":\"get_status\",\"request_id\":\"g1\"}\n") != 0) {
        return 21;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        return 22;
    }
    if (receive_response(&client, response, sizeof(response)) != 0) {
        return 23;
    }
    if (expect_contains(response, "\"type\":\"status\"") ||
        expect_contains(response, "\"present\":true") ||
        expect_contains(response, "\"buttons\":8") || expect_contains(response, "\"lx\":1234")) {
        return 24;
    }

    swbt_ipc_connection_configure_heartbeat(&connection, (swbt_ipc_heartbeat_config_t){
                                                             .now_ms = 1000u,
                                                             .timeout_ms = 100u,
                                                         });
    if (send_line(&client, "{\"v\":1,\"type\":\"get_status\",\"request_id\":\"g2\"}\n") != 0) {
        return 25;
    }
    if (swbt_ipc_server_serve_connection_once_at(&server, &connection, 1050u) !=
        SWBT_IPC_SERVER_OK) {
        return 26;
    }
    if (receive_response(&client, response, sizeof(response)) != 0) {
        return 27;
    }
    if (expect_contains(response, "\"type\":\"status\"") ||
        expect_contains(response, "\"request_id\":\"g2\"")) {
        return 28;
    }
    if (swbt_ipc_server_check_heartbeat(&server, &connection, 1149u) != SWBT_IPC_SERVER_OK) {
        return 29;
    }
    if (swbt_ipc_server_get_status(&server, &status) != SWBT_IPC_SERVER_OK) {
        return 30;
    }
    if (!status.has_owner || status.state.buttons != SWBT_BUTTON_A || status.state.lx != 1234u) {
        return 31;
    }
    if (swbt_ipc_server_check_heartbeat(&server, &connection, 1150u) !=
        SWBT_IPC_SERVER_ERROR_HEARTBEAT_TIMEOUT) {
        return 32;
    }
    if (swbt_ipc_server_get_status(&server, &status) != SWBT_IPC_SERVER_OK) {
        return 33;
    }
    if (status.has_owner || status.state.buttons != 0u || status.state.lx != 2048u) {
        return 34;
    }

    swbt_ipc_socket_close(&client);
    if (swbt_ipc_server_serve_connection_once(&server, &connection) !=
        SWBT_IPC_SERVER_ERROR_DISCONNECTED) {
        return 35;
    }
    if (swbt_ipc_server_get_status(&server, &status) != SWBT_IPC_SERVER_OK) {
        return 36;
    }
    if (status.has_owner || status.state.buttons != 0u || status.state.lx != 2048u) {
        return 37;
    }

    swbt_ipc_connection_close(&connection);
    swbt_ipc_server_close(&server);
    return 0;
}
