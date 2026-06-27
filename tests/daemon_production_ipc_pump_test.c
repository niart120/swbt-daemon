#include "daemon/production_ipc_pump.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int start_calls;
    int stop_calls;
    int start_result;
    bool runner_running_at_start;
    bool runner_running_at_stop;
    swbt_btstack_production_ipc_pump_t captured_pump;
    const swbt_daemon_ipc_runner_t *captured_runner;
} fake_pump_port_t;

static int expect_true(bool value, const char *label) {
    (void)label;
    return value ? 0 : 1;
}

static int expect_eq_int(int actual, int expected, const char *label) {
    (void)label;
    return actual == expected ? 0 : 1;
}

static int fake_pump_start(void *context, const swbt_btstack_production_ipc_pump_t *pump) {
    fake_pump_port_t *fake = context;
    fake->start_calls += 1;
    fake->runner_running_at_start =
        pump != NULL && pump->is_running != NULL && pump->is_running(pump->context);
    fake->captured_pump = pump == NULL ? (swbt_btstack_production_ipc_pump_t){0} : *pump;
    fake->captured_runner = pump == NULL ? NULL : (const swbt_daemon_ipc_runner_t *)pump->context;
    return fake->start_result;
}

static void fake_pump_stop(void *context) {
    fake_pump_port_t *fake = context;
    fake->stop_calls += 1;
    fake->runner_running_at_stop =
        fake->captured_runner != NULL && swbt_daemon_ipc_runner_is_running(fake->captured_runner);
}

static int start_starts_ipc_runner_before_btstack_pump(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    swbt_daemon_ipc_runner_t runner;
    fake_pump_port_t fake = {0};
    swbt_domain_t *app = swbt_domain_create();
    swbt_control_t control;
    const swbt_btstack_production_ipc_pump_port_t port = {
        .start = fake_pump_start,
        .stop = fake_pump_stop,
    };
    swbt_daemon_production_ipc_pump_t adapter;
    int failed = 0;

    failed += expect_eq_int(swbt_daemon_ipc_runner_init(&runner), SWBT_DAEMON_IPC_RUNNER_OK,
                            "runner init");
    runner.config = swbt_daemon_ipc_runner_config_from_daemon_config(&config);
    failed += expect_true(app != NULL, "app created");
    failed += expect_eq_int(swbt_control_init(&control,
                                              &(swbt_control_config_t){
                                                  .app = app,
                                              }),
                            SWBT_CONTROL_OK, "control init");

    adapter = (swbt_daemon_production_ipc_pump_t){
        .runner = &runner,
        .port = &port,
        .port_context = &fake,
    };

    failed +=
        expect_eq_int(swbt_daemon_production_ipc_pump_start(&adapter, &control), 0, "pump start");
    failed += expect_eq_int(fake.start_calls, 1, "pump start calls");
    failed += expect_true(fake.runner_running_at_start, "runner running at pump start");
    failed += expect_true(fake.captured_runner == &runner, "same runner instance");

    swbt_daemon_production_ipc_pump_stop(&adapter);
    swbt_domain_destroy(app);
    return failed;
}

static int start_failure_stops_ipc_runner(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    swbt_daemon_ipc_runner_t runner;
    fake_pump_port_t fake = {
        .start_result = -7,
    };
    swbt_domain_t *app = swbt_domain_create();
    swbt_control_t control;
    const swbt_btstack_production_ipc_pump_port_t port = {
        .start = fake_pump_start,
        .stop = fake_pump_stop,
    };
    swbt_daemon_production_ipc_pump_t adapter;
    int failed = 0;

    failed += expect_eq_int(swbt_daemon_ipc_runner_init(&runner), SWBT_DAEMON_IPC_RUNNER_OK,
                            "runner init");
    runner.config = swbt_daemon_ipc_runner_config_from_daemon_config(&config);
    failed += expect_true(app != NULL, "app created");
    failed += expect_eq_int(swbt_control_init(&control,
                                              &(swbt_control_config_t){
                                                  .app = app,
                                              }),
                            SWBT_CONTROL_OK, "control init");

    adapter = (swbt_daemon_production_ipc_pump_t){
        .runner = &runner,
        .port = &port,
        .port_context = &fake,
    };

    failed += expect_eq_int(swbt_daemon_production_ipc_pump_start(&adapter, &control), -1,
                            "pump start failure");
    failed += expect_eq_int(fake.start_calls, 1, "pump start calls");
    failed += expect_true(!swbt_daemon_ipc_runner_is_running(&runner), "runner stopped");
    failed += expect_eq_int(fake.stop_calls, 0, "pump stop not called");

    swbt_domain_destroy(app);
    return failed;
}

static int stop_stops_btstack_pump_before_ipc_runner(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    swbt_daemon_ipc_runner_t runner;
    fake_pump_port_t fake = {0};
    swbt_domain_t *app = swbt_domain_create();
    swbt_control_t control;
    const swbt_btstack_production_ipc_pump_port_t port = {
        .start = fake_pump_start,
        .stop = fake_pump_stop,
    };
    swbt_daemon_production_ipc_pump_t adapter;
    int failed = 0;

    failed += expect_eq_int(swbt_daemon_ipc_runner_init(&runner), SWBT_DAEMON_IPC_RUNNER_OK,
                            "runner init");
    runner.config = swbt_daemon_ipc_runner_config_from_daemon_config(&config);
    failed += expect_true(app != NULL, "app created");
    failed += expect_eq_int(swbt_control_init(&control,
                                              &(swbt_control_config_t){
                                                  .app = app,
                                              }),
                            SWBT_CONTROL_OK, "control init");

    adapter = (swbt_daemon_production_ipc_pump_t){
        .runner = &runner,
        .port = &port,
        .port_context = &fake,
    };

    failed +=
        expect_eq_int(swbt_daemon_production_ipc_pump_start(&adapter, &control), 0, "pump start");
    swbt_daemon_production_ipc_pump_stop(&adapter);
    failed += expect_eq_int(fake.stop_calls, 1, "pump stop calls");
    failed += expect_true(fake.runner_running_at_stop, "runner running at pump stop");
    failed += expect_true(!swbt_daemon_ipc_runner_is_running(&runner), "runner stopped after pump");

    swbt_domain_destroy(app);
    return failed;
}

static int callbacks_report_running_and_poll_same_runner(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    swbt_daemon_ipc_runner_t runner;
    fake_pump_port_t fake = {0};
    swbt_domain_t *app = swbt_domain_create();
    swbt_control_t control;
    const swbt_btstack_production_ipc_pump_port_t port = {
        .start = fake_pump_start,
        .stop = fake_pump_stop,
    };
    swbt_daemon_production_ipc_pump_t adapter;
    swbt_daemon_ipc_endpoint_t endpoint;
    swbt_ipc_socket_t client;
    bool client_open = false;
    int failed = 0;

    failed += expect_eq_int(swbt_daemon_ipc_runner_init(&runner), SWBT_DAEMON_IPC_RUNNER_OK,
                            "runner init");
    runner.config = swbt_daemon_ipc_runner_config_from_daemon_config(&config);
    failed += expect_true(app != NULL, "app created");
    failed += expect_eq_int(swbt_control_init(&control,
                                              &(swbt_control_config_t){
                                                  .app = app,
                                              }),
                            SWBT_CONTROL_OK, "control init");

    adapter = (swbt_daemon_production_ipc_pump_t){
        .runner = &runner,
        .port = &port,
        .port_context = &fake,
    };

    failed +=
        expect_eq_int(swbt_daemon_production_ipc_pump_start(&adapter, &control), 0, "pump start");
    failed += expect_true(fake.captured_pump.is_running != NULL, "is running callback");
    failed += expect_true(fake.captured_pump.poll_once_at != NULL, "poll callback");
    failed += expect_true(fake.captured_pump.is_running(fake.captured_pump.context),
                          "callback reports running");
    failed += expect_eq_int(swbt_daemon_ipc_runner_endpoint(&runner, &endpoint),
                            SWBT_DAEMON_IPC_RUNNER_OK, "endpoint");
    swbt_ipc_socket_init(&client);
    if (swbt_ipc_socket_connect_loopback(&client, endpoint.port) == SWBT_IPC_SERVER_OK) {
        client_open = true;
    }
    failed += expect_true(client_open, "client connected");
    fake.captured_pump.poll_once_at(fake.captured_pump.context, 1000u);
    failed += expect_true(swbt_daemon_ipc_runner_has_connection(&runner), "connection accepted");

    if (client_open) {
        swbt_ipc_socket_close(&client);
    }
    swbt_daemon_production_ipc_pump_stop(&adapter);
    swbt_domain_destroy(app);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += start_starts_ipc_runner_before_btstack_pump();
    failed += start_failure_stops_ipc_runner();
    failed += stop_stops_btstack_pump_before_ipc_runner();
    failed += callbacks_report_running_and_poll_same_runner();
    return failed == 0 ? 0 : 1;
}
