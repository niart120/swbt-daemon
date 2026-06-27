#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../apps/swbt-debug-client/debug_client.h"
#include "application/app.h"
#include "daemon/ipc_runner.h"
#include "ipc/ipc_server.h"
#include "switch/switch_controller_state.h"

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_false(bool value) {
    return value ? 1 : 0;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_contains(const char *text, const char *needle) {
    return strstr(text, needle) != NULL ? 0 : 1;
}

static swbt_daemon_ipc_runner_config_t loopback_port_zero_config(void) {
    return (swbt_daemon_ipc_runner_config_t){
        .host = "127.0.0.1",
        .port = 0u,
        .backlog = 1,
        .heartbeat_timeout_ms = 0u,
    };
}

static int send_and_serve(swbt_ipc_socket_t *client, swbt_daemon_ipc_runner_t *runner,
                          int (*send_request)(swbt_ipc_socket_t *)) {
    if (send_request(client) != 0) {
        return 1;
    }
    return swbt_daemon_ipc_runner_serve_connection_once(runner) == SWBT_DAEMON_IPC_RUNNER_OK ? 0
                                                                                             : 1;
}

static int test_rejects_non_loopback_bind(void) {
    swbt_daemon_ipc_runner_t runner;
    swbt_app_t *app = swbt_app_create();
    swbt_daemon_ipc_runner_config_t config = {
        .host = "0.0.0.0",
        .port = 0u,
        .backlog = 1,
    };

    int failed = 0;
    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_daemon_ipc_runner_init(&runner), SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_start(&runner, app, &config),
                            SWBT_DAEMON_IPC_RUNNER_ERROR_UNSUPPORTED_BIND);
    swbt_daemon_ipc_runner_stop(&runner);
    swbt_app_destroy(app);
    return failed;
}

static int test_exposes_loopback_endpoint_before_accept(void) {
    swbt_daemon_ipc_runner_t runner;
    swbt_app_t *app = swbt_app_create();
    swbt_daemon_ipc_endpoint_t endpoint;
    const swbt_daemon_ipc_runner_config_t config = loopback_port_zero_config();

    int failed = 0;
    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_daemon_ipc_runner_init(&runner), SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_start(&runner, app, &config),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_endpoint(&runner, &endpoint),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_true(endpoint.bound);
    failed += expect_eq_int(strcmp(endpoint.host, "127.0.0.1"), 0);
    failed += expect_true(endpoint.port != 0u);
    failed += expect_false(swbt_daemon_ipc_runner_has_connection(&runner));

    swbt_daemon_ipc_runner_stop(&runner);
    swbt_app_destroy(app);
    return failed;
}

static int test_poll_once_accepts_and_serves_when_ready(void) {
    swbt_daemon_ipc_runner_t runner;
    swbt_app_t *app = swbt_app_create();
    swbt_daemon_ipc_endpoint_t endpoint;
    swbt_ipc_socket_t client;
    char response[SWBT_IPC_JSON_RESPONSE_MAX];
    const swbt_daemon_ipc_runner_config_t config = loopback_port_zero_config();

    int failed = 0;
    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_daemon_ipc_runner_init(&runner), SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_start(&runner, app, &config),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_endpoint(&runner, &endpoint),
                            SWBT_DAEMON_IPC_RUNNER_OK);

    failed += expect_eq_int(swbt_daemon_ipc_runner_poll_once(&runner), SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_false(swbt_daemon_ipc_runner_has_connection(&runner));

    swbt_ipc_socket_init(&client);
    failed +=
        expect_eq_int(swbt_ipc_socket_connect_loopback(&client, endpoint.port), SWBT_IPC_SERVER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_poll_once(&runner), SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_true(swbt_daemon_ipc_runner_has_connection(&runner));

    failed += expect_eq_int(swbt_daemon_ipc_runner_poll_once(&runner), SWBT_DAEMON_IPC_RUNNER_OK);

    failed += expect_eq_int(swbt_debug_client_send_hello(&client), 0);
    failed += expect_eq_int(swbt_daemon_ipc_runner_poll_once(&runner), SWBT_DAEMON_IPC_RUNNER_OK);
    failed +=
        expect_eq_int(swbt_debug_client_receive_response(&client, response, sizeof(response)), 0);
    failed += expect_contains(response, "\"type\":\"hello_ok\"");

    swbt_ipc_socket_close(&client);
    swbt_daemon_ipc_runner_stop(&runner);
    swbt_app_destroy(app);
    return failed;
}

static int test_debug_client_status_sequence_updates_application_state(void) {
    swbt_daemon_ipc_runner_t runner;
    swbt_app_t *app = swbt_app_create();
    swbt_state_t read_state;
    swbt_daemon_ipc_endpoint_t endpoint;
    swbt_ipc_socket_t client;
    swbt_state_t state = swbt_state_neutral();
    char owner_id[9];
    char response[SWBT_IPC_JSON_RESPONSE_MAX];
    const swbt_daemon_ipc_runner_config_t config = loopback_port_zero_config();

    int failed = 0;
    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_daemon_ipc_runner_init(&runner), SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_start(&runner, app, &config),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_endpoint(&runner, &endpoint),
                            SWBT_DAEMON_IPC_RUNNER_OK);

    swbt_ipc_socket_init(&client);
    failed +=
        expect_eq_int(swbt_ipc_socket_connect_loopback(&client, endpoint.port), SWBT_IPC_SERVER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_accept(&runner), SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_true(swbt_daemon_ipc_runner_has_connection(&runner));

    failed += send_and_serve(&client, &runner, swbt_debug_client_send_hello);
    failed +=
        expect_eq_int(swbt_debug_client_receive_response(&client, response, sizeof(response)), 0);
    failed += expect_contains(response, "\"type\":\"hello_ok\"");

    failed += send_and_serve(&client, &runner, swbt_debug_client_send_acquire);
    failed +=
        expect_eq_int(swbt_debug_client_receive_response(&client, response, sizeof(response)), 0);
    failed += expect_contains(response, "\"type\":\"acquired\"");
    failed += expect_eq_int(
        swbt_debug_client_response_string(response, "owner_id", owner_id, sizeof(owner_id)), 0);

    state.buttons = SWBT_BUTTON_A | SWBT_BUTTON_X;
    state.lx = 1234u;
    state.ly = 2345u;
    failed +=
        expect_eq_int(swbt_debug_client_send_set_state(&client,
                                                       (swbt_debug_client_set_state_options_t){
                                                           .owner_id = owner_id,
                                                           .state = &state,
                                                           .sequence = 42u,
                                                       }),
                      0);
    failed += expect_eq_int(swbt_daemon_ipc_runner_serve_connection_once(&runner),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed +=
        expect_eq_int(swbt_debug_client_receive_response(&client, response, sizeof(response)), 0);
    failed += expect_contains(response, "\"type\":\"state_accepted\"");
    failed += expect_eq_int(swbt_app_read_controller_state(app, &read_state), SWBT_APP_OK);
    failed += expect_eq_u32(read_state.buttons, SWBT_BUTTON_A | SWBT_BUTTON_X);
    failed += expect_eq_u16(read_state.lx, 1234u);
    failed += expect_eq_u16(read_state.ly, 2345u);

    failed += send_and_serve(&client, &runner, swbt_debug_client_send_get_status);
    failed +=
        expect_eq_int(swbt_debug_client_receive_response(&client, response, sizeof(response)), 0);
    failed += expect_contains(response, "\"type\":\"status\"");
    failed += expect_contains(response, "\"last_seq\":42");

    swbt_ipc_socket_close(&client);
    swbt_daemon_ipc_runner_stop(&runner);
    swbt_app_destroy(app);
    return failed;
}

static int test_stop_closes_connection_and_stores_neutral(void) {
    swbt_daemon_ipc_runner_t runner;
    swbt_app_t *app = swbt_app_create();
    swbt_state_t read_state;
    swbt_daemon_ipc_endpoint_t endpoint;
    swbt_ipc_socket_t client;
    swbt_state_t state = swbt_state_neutral();
    char owner_id[9];
    char response[SWBT_IPC_JSON_RESPONSE_MAX];
    const swbt_daemon_ipc_runner_config_t config = loopback_port_zero_config();

    int failed = 0;
    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_daemon_ipc_runner_init(&runner), SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_start(&runner, app, &config),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_endpoint(&runner, &endpoint),
                            SWBT_DAEMON_IPC_RUNNER_OK);

    swbt_ipc_socket_init(&client);
    failed +=
        expect_eq_int(swbt_ipc_socket_connect_loopback(&client, endpoint.port), SWBT_IPC_SERVER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_accept(&runner), SWBT_DAEMON_IPC_RUNNER_OK);
    failed += send_and_serve(&client, &runner, swbt_debug_client_send_hello);
    failed +=
        expect_eq_int(swbt_debug_client_receive_response(&client, response, sizeof(response)), 0);
    failed += send_and_serve(&client, &runner, swbt_debug_client_send_acquire);
    failed +=
        expect_eq_int(swbt_debug_client_receive_response(&client, response, sizeof(response)), 0);
    failed += expect_eq_int(
        swbt_debug_client_response_string(response, "owner_id", owner_id, sizeof(owner_id)), 0);

    state.buttons = SWBT_BUTTON_A;
    failed +=
        expect_eq_int(swbt_debug_client_send_set_state(&client,
                                                       (swbt_debug_client_set_state_options_t){
                                                           .owner_id = owner_id,
                                                           .state = &state,
                                                           .sequence = 7u,
                                                       }),
                      0);
    failed += expect_eq_int(swbt_daemon_ipc_runner_serve_connection_once(&runner),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed +=
        expect_eq_int(swbt_debug_client_receive_response(&client, response, sizeof(response)), 0);

    swbt_daemon_ipc_runner_stop(&runner);
    failed += expect_false(swbt_daemon_ipc_runner_is_running(&runner));
    failed += expect_false(swbt_daemon_ipc_runner_has_connection(&runner));
    failed += expect_eq_int(swbt_app_read_controller_state(app, &read_state), SWBT_APP_OK);
    failed += expect_eq_u32(read_state.buttons, 0u);
    failed += expect_eq_u16(read_state.lx, 2048u);

    swbt_ipc_socket_close(&client);
    swbt_app_destroy(app);
    return failed;
}

static int test_poll_once_at_heartbeat_timeout_stores_neutral(void) {
    swbt_daemon_ipc_runner_t runner;
    swbt_app_t *app = swbt_app_create();
    swbt_state_t read_state;
    swbt_daemon_ipc_endpoint_t endpoint;
    swbt_ipc_socket_t client;
    swbt_state_t state = swbt_state_neutral();
    char owner_id[9];
    char response[SWBT_IPC_JSON_RESPONSE_MAX];
    swbt_daemon_ipc_runner_config_t config = loopback_port_zero_config();

    int failed = 0;
    config.heartbeat_timeout_ms = 100u;

    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_daemon_ipc_runner_init(&runner), SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_start(&runner, app, &config),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_endpoint(&runner, &endpoint),
                            SWBT_DAEMON_IPC_RUNNER_OK);

    swbt_ipc_socket_init(&client);
    failed +=
        expect_eq_int(swbt_ipc_socket_connect_loopback(&client, endpoint.port), SWBT_IPC_SERVER_OK);
    failed += expect_eq_int(swbt_daemon_ipc_runner_poll_once_at(&runner, 1000u),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_true(swbt_daemon_ipc_runner_has_connection(&runner));

    failed += expect_eq_int(swbt_debug_client_send_hello(&client), 0);
    failed += expect_eq_int(swbt_daemon_ipc_runner_poll_once_at(&runner, 1010u),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed +=
        expect_eq_int(swbt_debug_client_receive_response(&client, response, sizeof(response)), 0);

    failed += expect_eq_int(swbt_debug_client_send_acquire(&client), 0);
    failed += expect_eq_int(swbt_daemon_ipc_runner_poll_once_at(&runner, 1020u),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed +=
        expect_eq_int(swbt_debug_client_receive_response(&client, response, sizeof(response)), 0);
    failed += expect_eq_int(
        swbt_debug_client_response_string(response, "owner_id", owner_id, sizeof(owner_id)), 0);

    state.buttons = SWBT_BUTTON_A;
    failed +=
        expect_eq_int(swbt_debug_client_send_set_state(&client,
                                                       (swbt_debug_client_set_state_options_t){
                                                           .owner_id = owner_id,
                                                           .state = &state,
                                                           .sequence = 9u,
                                                       }),
                      0);
    failed += expect_eq_int(swbt_daemon_ipc_runner_poll_once_at(&runner, 1050u),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed +=
        expect_eq_int(swbt_debug_client_receive_response(&client, response, sizeof(response)), 0);
    failed += expect_eq_int(swbt_app_read_controller_state(app, &read_state), SWBT_APP_OK);
    failed += expect_eq_u32(read_state.buttons, SWBT_BUTTON_A);

    failed += expect_eq_int(swbt_daemon_ipc_runner_poll_once_at(&runner, 1149u),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_true(swbt_daemon_ipc_runner_has_connection(&runner));
    failed += expect_eq_int(swbt_app_read_controller_state(app, &read_state), SWBT_APP_OK);
    failed += expect_eq_u32(read_state.buttons, SWBT_BUTTON_A);

    failed += expect_eq_int(swbt_daemon_ipc_runner_poll_once_at(&runner, 1150u),
                            SWBT_DAEMON_IPC_RUNNER_OK);
    failed += expect_false(swbt_daemon_ipc_runner_has_connection(&runner));
    failed += expect_eq_int(swbt_app_read_controller_state(app, &read_state), SWBT_APP_OK);
    failed += expect_eq_u32(read_state.buttons, 0u);
    failed += expect_eq_u16(read_state.lx, 2048u);

    swbt_ipc_socket_close(&client);
    swbt_daemon_ipc_runner_stop(&runner);
    swbt_app_destroy(app);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += test_rejects_non_loopback_bind();
    failed += test_exposes_loopback_endpoint_before_accept();
    failed += test_poll_once_accepts_and_serves_when_ready();
    failed += test_debug_client_status_sequence_updates_application_state();
    failed += test_stop_closes_connection_and_stores_neutral();
    failed += test_poll_once_at_heartbeat_timeout_stores_neutral();
    return failed == 0 ? 0 : 1;
}
