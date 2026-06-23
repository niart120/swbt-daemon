#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "application/app.h"
#include "btstack_bridge/output_report_handler.h"
#include "daemon/config.h"
#include "daemon/host.h"
#include "ipc/ipc_adapter.h"
#include "ipc/ipc_json.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_report.h"

typedef struct {
    int ipc_start_calls;
    int ipc_stop_calls;
    int hid_register_calls;
    int hid_stop_calls;
    int output_handler_start_calls;
    int output_handler_stop_calls;
    int report_timer_start_calls;
    int report_timer_stop_calls;
    swbt_app_t *app;
    swbt_daemon_host_state_provider_t state_provider;
    void *state_context;
} fake_backend_t;

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_contains(const char *text, const char *needle) {
    const char *cursor = text;
    if (text == NULL || needle == NULL) {
        return 1;
    }
    while (*cursor != '\0') {
        const char *lhs = cursor;
        const char *rhs = needle;
        while (*lhs != '\0' && *rhs != '\0' && *lhs == *rhs) {
            ++lhs;
            ++rhs;
        }
        if (*rhs == '\0') {
            return 0;
        }
        ++cursor;
    }
    return 1;
}

static int fake_ipc_start(void *context, swbt_app_t *app) {
    fake_backend_t *fake = context;
    fake->ipc_start_calls += 1;
    fake->app = app;
    return 0;
}

static void fake_ipc_stop(void *context) {
    fake_backend_t *fake = context;
    fake->ipc_stop_calls += 1;
}

static int fake_hid_register(void *context) {
    fake_backend_t *fake = context;
    fake->hid_register_calls += 1;
    return 0;
}

static void fake_hid_stop(void *context) {
    fake_backend_t *fake = context;
    fake->hid_stop_calls += 1;
}

static void fake_output_handler_start(void *context,
                                      swbt_btstack_output_report_handler_t *handler) {
    fake_backend_t *fake = context;
    (void)handler;
    fake->output_handler_start_calls += 1;
}

static void fake_output_handler_stop(void *context) {
    fake_backend_t *fake = context;
    fake->output_handler_stop_calls += 1;
}

static int fake_report_timer_start(void *context, swbt_daemon_host_state_provider_t state_provider,
                                   void *state_context) {
    fake_backend_t *fake = context;
    fake->report_timer_start_calls += 1;
    fake->state_provider = state_provider;
    fake->state_context = state_context;
    return 0;
}

static void fake_report_timer_stop(void *context) {
    fake_backend_t *fake = context;
    fake->report_timer_stop_calls += 1;
}

static int fake_report_timer_send_neutral_now(void *context) {
    (void)context;
    return 0;
}

static int fake_subcommand_reply_enqueue(void *context, uint16_t hid_cid, const uint8_t *report,
                                         size_t report_size) {
    (void)context;
    (void)hid_cid;
    (void)report;
    (void)report_size;
    return 0;
}

static uint32_t fake_time_ms(void *context) {
    (void)context;
    return 123u;
}

static swbt_daemon_host_backend_t fake_backend(void) {
    return (swbt_daemon_host_backend_t){
        .daemon_backend = SWBT_APP_DAEMON_BACKEND_NOOP,
        .ipc_start = fake_ipc_start,
        .ipc_stop = fake_ipc_stop,
        .hid_register = fake_hid_register,
        .hid_stop = fake_hid_stop,
        .output_handler_start = fake_output_handler_start,
        .output_handler_stop = fake_output_handler_stop,
        .report_timer_start = fake_report_timer_start,
        .report_timer_stop = fake_report_timer_stop,
        .report_timer_send_neutral_now = fake_report_timer_send_neutral_now,
        .subcommand_reply_enqueue = fake_subcommand_reply_enqueue,
        .time_ms = fake_time_ms,
    };
}

static int handle(swbt_app_t *app, uint32_t client_id, const char *line, char *response,
                  size_t response_size) {
    for (size_t index = 0; index < response_size; ++index) {
        response[index] = '\0';
    }
    return swbt_ipc_adapter_handle_line(app, client_id, line, response, response_size);
}

static int send_button_a_state(swbt_app_t *app, uint32_t client_id, uint64_t sequence,
                               const char *request_id) {
    char line[256];
    char response[SWBT_IPC_JSON_RESPONSE_MAX];

    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    if (snprintf(line, sizeof(line),
                 "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"%08x\",\"seq\":%llu,"
                 "\"request_id\":\"%s\",\"state\":{\"buttons\":8,\"lx\":2048,\"ly\":2048,"
                 "\"rx\":2048,\"ry\":2048,\"accel_x\":0,\"accel_y\":0,\"accel_z\":0,"
                 "\"gyro_x\":0,\"gyro_y\":0,\"gyro_z\":0}}\n",
                 client_id, (unsigned long long)sequence, request_id) < 0) {
        return 1;
    }
    if (handle(app, client_id, line, response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 1;
    }
    return expect_contains(response, "\"type\":\"state_accepted\"");
}

static int expect_current_report_button_byte(const fake_backend_t *fake,
                                             const swbt_daemon_config_t *config,
                                             uint8_t expected_button_byte) {
    uint8_t report[SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE];
    size_t written = 0u;
    swbt_state_t state;
    swbt_switch_report_options_t options;

    if (fake == NULL || fake->state_provider == NULL || config == NULL) {
        return 1;
    }

    state = fake->state_provider(fake->state_context);
    options = config->report_options;
    options.timer = 0x42u;
    if (swbt_switch_build_standard_full_report(&state, &options, report, sizeof(report),
                                               &written) != SWBT_SWITCH_REPORT_OK) {
        return 2;
    }
    if (written != SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE) {
        return 3;
    }
    return expect_eq_u8(report[3], expected_button_byte);
}

static int expect_status(swbt_app_t *app, bool has_owner, uint32_t owner_client_id,
                         uint32_t buttons) {
    swbt_ipc_status_t status;
    int failed = 0;

    failed += expect_eq_int(swbt_ipc_adapter_get_status(app, &status), SWBT_IPC_OK);
    failed += expect_true(status.has_owner == has_owner);
    if (has_owner) {
        failed += expect_eq_u32(status.owner_client_id, owner_client_id);
    }
    failed += expect_eq_u32(status.state.buttons, buttons);
    return failed;
}

static int architecture_journey_neutralizes_disconnect_and_shutdown(void) {
    swbt_daemon_host_t host;
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_backend_t fake = {0};
    const swbt_daemon_host_backend_t backend = fake_backend();
    char response[SWBT_IPC_JSON_RESPONSE_MAX];
    int failed = 0;

    failed +=
        expect_eq_int(swbt_daemon_host_init(&host, &config, &backend, &fake), SWBT_DAEMON_HOST_OK);
    failed += expect_eq_int(swbt_daemon_host_start(&host), SWBT_DAEMON_HOST_OK);
    failed += expect_eq_int(fake.ipc_start_calls, 1);
    failed += expect_eq_int(fake.hid_register_calls, 1);
    failed += expect_eq_int(fake.output_handler_start_calls, 1);
    failed += expect_eq_int(fake.report_timer_start_calls, 1);
    failed += expect_true(fake.app == swbt_daemon_host_app(&host));

    failed += expect_eq_int(handle(fake.app, 1001u,
                                   "{\"v\":1,\"type\":\"acquire\",\"mode\":\"exclusive\","
                                   "\"request_id\":\"a1\"}\n",
                                   response, sizeof(response)),
                            SWBT_IPC_JSON_OK);
    failed += expect_contains(response, "\"type\":\"acquired\"");
    failed += send_button_a_state(fake.app, 1001u, 1u, "s1");
    failed += expect_status(fake.app, true, 1001u, SWBT_BUTTON_A);
    failed += expect_current_report_button_byte(&fake, &config, 0x08u);

    failed += expect_eq_int(swbt_ipc_adapter_handle_disconnect(fake.app, 1001u), SWBT_IPC_OK);
    failed += expect_status(fake.app, false, 0u, 0u);
    failed += expect_current_report_button_byte(&fake, &config, 0x00u);

    failed += expect_eq_int(handle(fake.app, 2002u,
                                   "{\"v\":1,\"type\":\"acquire\",\"mode\":\"exclusive\","
                                   "\"request_id\":\"a2\"}\n",
                                   response, sizeof(response)),
                            SWBT_IPC_JSON_OK);
    failed += expect_contains(response, "\"type\":\"acquired\"");
    failed += send_button_a_state(fake.app, 2002u, 2u, "s2");
    failed += expect_status(fake.app, true, 2002u, SWBT_BUTTON_A);
    failed += expect_current_report_button_byte(&fake, &config, 0x08u);

    swbt_daemon_host_stop(&host);
    failed += expect_eq_int(fake.report_timer_stop_calls, 1);
    failed += expect_eq_int(fake.output_handler_stop_calls, 1);
    failed += expect_eq_int(fake.hid_stop_calls, 1);
    failed += expect_eq_int(fake.ipc_stop_calls, 1);
    failed += expect_status(fake.app, false, 0u, 0u);
    failed += expect_current_report_button_byte(&fake, &config, 0x00u);

    swbt_daemon_host_destroy(&host);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += architecture_journey_neutralizes_disconnect_and_shutdown();
    return failed == 0 ? 0 : 1;
}
