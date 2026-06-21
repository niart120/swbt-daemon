#include <stdio.h>
#include <string.h>

#include "../apps/swbt-debug-client/debug_client.h"

static int expect_zero(int value) {
    return value == 0 ? 0 : 1;
}

static int expect_nonzero(int value) {
    return value != 0 ? 0 : 1;
}

static int expect_contains(const char *text, const char *needle) {
    return strstr(text, needle) != NULL ? 0 : 1;
}

static int test_invalid_state_value_is_rejected(void) {
    const char *invalid_lx[] = {
        "swbt-debug-client", "--port", "9", "--lx", "4096",
    };

    if (expect_nonzero(swbt_debug_client_main(5, invalid_lx, stdout, stderr))) {
        return 1;
    }

    return 0;
}

static int test_loopback_hello_send_receive(void) {
    swbt_ipc_server_t server;
    swbt_ipc_connection_t connection;
    swbt_ipc_socket_t client;
    swbt_ipc_status_t status;
    swbt_state_t state = swbt_state_neutral();
    char owner_id[9];
    char response[SWBT_IPC_JSON_RESPONSE_MAX];

    if (swbt_ipc_server_init(&server) != SWBT_IPC_SERVER_OK) {
        return 10;
    }
    if (swbt_ipc_server_listen(&server, "127.0.0.1", 0, 1) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_server_close(&server);
        return 11;
    }

    swbt_ipc_socket_init(&client);
    if (swbt_ipc_socket_connect_loopback(&client, swbt_ipc_server_port(&server)) !=
        SWBT_IPC_SERVER_OK) {
        swbt_ipc_server_close(&server);
        return 12;
    }
    if (swbt_ipc_server_accept(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 13;
    }

    if (expect_zero(swbt_debug_client_send_hello(&client))) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 14;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 15;
    }
    if (expect_zero(swbt_debug_client_receive_response(&client, response, sizeof(response)))) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 16;
    }
    if (expect_contains(response, "\"type\":\"hello_ok\"") ||
        expect_contains(response, "\"client_id\":\"00000001\"")) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 17;
    }

    if (expect_zero(swbt_debug_client_send_acquire(&client))) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 18;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 19;
    }
    if (expect_zero(swbt_debug_client_receive_response(&client, response, sizeof(response)))) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 20;
    }
    if (expect_contains(response, "\"type\":\"acquired\"") ||
        expect_zero(
            swbt_debug_client_response_string(response, "owner_id", owner_id, sizeof(owner_id))) ||
        strcmp(owner_id, "00000001") != 0) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 21;
    }

    state.buttons = SWBT_BUTTON_A;
    state.lx = 1234;
    state.ly = 2345;
    state.client_seq = 42;
    if (expect_zero(swbt_debug_client_send_set_state(&client, owner_id, &state))) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 22;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 23;
    }
    if (expect_zero(swbt_debug_client_receive_response(&client, response, sizeof(response)))) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 24;
    }
    if (expect_contains(response, "\"type\":\"state_accepted\"") ||
        expect_contains(response, "\"seq\":42")) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 25;
    }

    if (expect_zero(swbt_debug_client_send_get_status(&client))) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 26;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 27;
    }
    if (expect_zero(swbt_debug_client_receive_response(&client, response, sizeof(response)))) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 28;
    }
    if (expect_contains(response, "\"type\":\"status\"") ||
        expect_contains(response, "\"buttons\":8") || expect_contains(response, "\"lx\":1234") ||
        expect_contains(response, "\"last_seq\":42")) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 29;
    }

    if (expect_zero(swbt_debug_client_send_release(&client, owner_id))) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 30;
    }
    if (swbt_ipc_server_serve_connection_once(&server, &connection) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 31;
    }
    if (expect_zero(swbt_debug_client_receive_response(&client, response, sizeof(response)))) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 32;
    }
    if (expect_contains(response, "\"type\":\"released\"")) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 33;
    }
    if (swbt_ipc_server_get_status(&server, &status) != SWBT_IPC_SERVER_OK) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 34;
    }
    if (status.has_owner || status.state.buttons != 0u || status.state.lx != 2048u) {
        swbt_ipc_connection_close(&connection);
        swbt_ipc_socket_close(&client);
        swbt_ipc_server_close(&server);
        return 35;
    }

    swbt_ipc_connection_close(&connection);
    swbt_ipc_socket_close(&client);
    swbt_ipc_server_close(&server);
    return 0;
}

typedef struct {
    const char *responses[8];
    size_t response_count;
    size_t response_index;
    char sent[8][SWBT_IPC_JSON_RESPONSE_MAX];
    size_t sent_count;
} fake_io_t;

static int fake_send(void *context, const char *data, size_t size) {
    fake_io_t *fake = (fake_io_t *)context;
    if (fake->sent_count >= 8u || size >= SWBT_IPC_JSON_RESPONSE_MAX) {
        return -1;
    }
    for (size_t index = 0; index < size; ++index) {
        fake->sent[fake->sent_count][index] = data[index];
    }
    fake->sent[fake->sent_count][size] = '\0';
    ++fake->sent_count;
    return 0;
}

static int fake_receive(void *context, char *buffer, size_t buffer_size) {
    fake_io_t *fake = (fake_io_t *)context;
    const char *response = NULL;
    size_t response_size = 0;
    if (fake->response_index >= fake->response_count) {
        return -1;
    }
    response = fake->responses[fake->response_index];
    response_size = strlen(response);
    if (response_size + 1u > buffer_size) {
        return -1;
    }
    for (size_t index = 0; index <= response_size; ++index) {
        buffer[index] = response[index];
    }
    ++fake->response_index;
    return 0;
}

static int test_connection_failure_returns_nonzero(void) {
    const char *argv[] = {
        "swbt-debug-client", "--port", "9", "--button", "a",
    };
    return expect_nonzero(swbt_debug_client_main(5, argv, stdout, stderr));
}

static int test_unsupported_timing_macro_argument_is_rejected(void) {
    const char *argv[] = {
        "swbt-debug-client", "--port", "9", "--duration-ms", "100",
    };
    return swbt_debug_client_main(5, argv, stdout, stderr) == 2 ? 0 : 1;
}

static int test_owner_busy_stops_before_set_state(void) {
    swbt_debug_client_config_t config = {
        .port = 9,
        .state = {0},
    };
    fake_io_t fake = {
        .responses =
            {
                "{\"v\":1,\"type\":\"hello_ok\",\"request_id\":\"h1\",\"client_id\":\"00000002\"}"
                "\n",
                "{\"v\":1,\"type\":\"error\",\"request_id\":\"a1\",\"code\":\"owner_busy\","
                "\"message\":\"another client owns the controller\"}\n",
            },
        .response_count = 2,
    };
    swbt_debug_client_io_t io = {
        .send = fake_send,
        .receive = fake_receive,
        .context = &fake,
    };
    config.state = swbt_state_neutral();
    config.state.buttons = SWBT_BUTTON_A;

    if (expect_nonzero(swbt_debug_client_run_io(&config, &io, stdout, stderr))) {
        return 1;
    }
    if (fake.sent_count != 2u) {
        return 2;
    }
    if (expect_contains(fake.sent[0], "\"type\":\"hello\"") ||
        expect_contains(fake.sent[1], "\"type\":\"acquire\"")) {
        return 3;
    }
    return 0;
}

static int test_owner_acquired_error_path_sends_release(void) {
    swbt_debug_client_config_t config = {
        .port = 9,
        .state = {0},
    };
    fake_io_t fake = {
        .responses =
            {
                "{\"v\":1,\"type\":\"hello_ok\",\"request_id\":\"h1\",\"client_id\":\"00000001\"}"
                "\n",
                "{\"v\":1,\"type\":\"acquired\",\"request_id\":\"a1\",\"owner_id\":\"00000001\"}\n",
                "{\"v\":1,\"type\":\"error\",\"request_id\":\"s1\",\"code\":\"invalid_state\","
                "\"message\":\"state field is invalid\"}\n",
                "{\"v\":1,\"type\":\"released\",\"request_id\":\"r1\"}\n",
            },
        .response_count = 4,
    };
    swbt_debug_client_io_t io = {
        .send = fake_send,
        .receive = fake_receive,
        .context = &fake,
    };
    config.state = swbt_state_neutral();
    config.state.buttons = SWBT_BUTTON_A;

    if (expect_nonzero(swbt_debug_client_run_io(&config, &io, stdout, stderr))) {
        return 1;
    }
    if (fake.sent_count != 4u) {
        return 2;
    }
    if (expect_contains(fake.sent[0], "\"type\":\"hello\"") ||
        expect_contains(fake.sent[1], "\"type\":\"acquire\"") ||
        expect_contains(fake.sent[2], "\"type\":\"set_state\"") ||
        expect_contains(fake.sent[3], "\"type\":\"release\"")) {
        return 3;
    }
    return 0;
}

static int test_skip_release_closes_without_release_request(void) {
    swbt_debug_client_config_t config = {
        .port = 9,
        .state = {0},
        .skip_release = true,
    };
    fake_io_t fake = {
        .responses =
            {
                "{\"v\":1,\"type\":\"hello_ok\",\"request_id\":\"h1\",\"client_id\":\"00000001\"}"
                "\n",
                "{\"v\":1,\"type\":\"acquired\",\"request_id\":\"a1\",\"owner_id\":\"00000001\"}\n",
                "{\"v\":1,\"type\":\"state_accepted\",\"request_id\":\"s1\",\"seq\":7}\n",
                "{\"v\":1,\"type\":\"status\",\"request_id\":\"q1\",\"owner\":{\"present\":true,"
                "\"owner_id\":\"00000001\",\"last_seq\":7},\"state\":{\"buttons\":8}}\n",
            },
        .response_count = 4,
    };
    swbt_debug_client_io_t io = {
        .send = fake_send,
        .receive = fake_receive,
        .context = &fake,
    };
    config.state = swbt_state_neutral();
    config.state.buttons = SWBT_BUTTON_A;
    config.state.client_seq = 7u;

    if (expect_zero(swbt_debug_client_run_io(&config, &io, stdout, stderr))) {
        return 1;
    }
    if (fake.sent_count != 4u) {
        return 2;
    }
    if (expect_contains(fake.sent[0], "\"type\":\"hello\"") ||
        expect_contains(fake.sent[1], "\"type\":\"acquire\"") ||
        expect_contains(fake.sent[2], "\"type\":\"set_state\"") ||
        expect_contains(fake.sent[3], "\"type\":\"get_status\"")) {
        return 3;
    }
    if (strstr(fake.sent[0], "\"type\":\"release\"") != NULL ||
        strstr(fake.sent[1], "\"type\":\"release\"") != NULL ||
        strstr(fake.sent[2], "\"type\":\"release\"") != NULL ||
        strstr(fake.sent[3], "\"type\":\"release\"") != NULL) {
        return 4;
    }
    return 0;
}

int main(void) {
    if (test_invalid_state_value_is_rejected() != 0) {
        return 1;
    }
    if (test_loopback_hello_send_receive() != 0) {
        return 2;
    }
    if (test_connection_failure_returns_nonzero() != 0) {
        return 3;
    }
    if (test_unsupported_timing_macro_argument_is_rejected() != 0) {
        return 4;
    }
    if (test_owner_busy_stops_before_set_state() != 0) {
        return 5;
    }
    if (test_owner_acquired_error_path_sends_release() != 0) {
        return 6;
    }
    if (test_skip_release_closes_without_release_request() != 0) {
        return 7;
    }

    return 0;
}
