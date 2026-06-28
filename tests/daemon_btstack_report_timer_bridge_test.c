#include "daemon/btstack_report_timer_bridge.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "btstack_bridge/device.h"
#include "btstack_bridge/hid_device_registration.h"
#include "btstack_bridge/production_ports.h"
#include "daemon/config.h"
#include "daemon/process_internal.h"
#include "domain/domain.h"
#include "switch/switch_controller_state.h"

typedef struct {
    int device_send_calls;
    int report_timer_init_calls;
    int timer_send_neutral_now_calls;
    int timer_send_neutral_now_result;
    int enqueue_reply_calls;
    uint16_t last_device_hid_cid;
    uint16_t last_reply_hid_cid;
    size_t last_device_message_size;
    size_t last_reply_size;
    uint8_t last_device_message[8];
    uint8_t last_reply[8];
    swbt_btstack_input_report_timer_adapter_config_t captured_timer_config;
} fake_ops_t;

typedef struct {
    swbt_btstack_input_report_timer_report_send_result_t send_result;
    int expected_ok;
    int expected_failed;
} report_tick_metrics_case_t;

static int expect_true(bool value, const char *label) {
    if (!value) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "expected true: %s\n", label);
        return 1;
    }
    return 0;
}

static int expect_eq_int(int actual, int expected, const char *label) {
    if (actual != expected) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %d, got %d\n", label, expected, actual);
        return 1;
    }
    return 0;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected, const char *label) {
    if (actual != expected) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %u, got %u\n", label, (unsigned)expected, (unsigned)actual);
        return 1;
    }
    return 0;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected, const char *label) {
    if (actual != expected) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected 0x%02x, got 0x%02x\n", label, (unsigned)expected,
                (unsigned)actual);
        return 1;
    }
    return 0;
}

static int fake_device_platform_start(void *context) {
    (void)context;
    return 0;
}

static void fake_device_platform_stop(void *context) {
    (void)context;
}

static int fake_hid_register(void *context, uint8_t *service_buffer, size_t service_buffer_size,
                             const swbt_btstack_hid_registration_config_t *config) {
    (void)context;
    (void)service_buffer;
    (void)service_buffer_size;
    (void)config;
    return 0;
}

static void fake_hid_stop(void *context) {
    (void)context;
}

static int fake_device_connect(void *context, const swbt_btstack_device_connect_request_t *request,
                               uint16_t *out_hid_cid) {
    (void)context;
    (void)request;
    *out_hid_cid = 0x0042u;
    return 0;
}

static int fake_device_send(void *context, uint16_t hid_cid, const uint8_t *message,
                            size_t message_size) {
    fake_ops_t *fake = context;
    fake->device_send_calls += 1;
    fake->last_device_hid_cid = hid_cid;
    fake->last_device_message_size = message_size;
    if (message_size <= sizeof(fake->last_device_message)) {
        // The fake capture buffer capacity is checked before copying message bytes.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        (void)memcpy(fake->last_device_message, message, message_size);
    }
    return 0;
}

static int fake_report_timer_init(void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
                                  const swbt_btstack_input_report_timer_adapter_config_t *config) {
    fake_ops_t *fake = context;
    fake->report_timer_init_calls += 1;
    fake->captured_timer_config = *config;
    (void)adapter;
    return 0;
}

static int fake_report_timer_start(void *context,
                                   swbt_btstack_input_report_timer_adapter_t *adapter,
                                   swbt_btstack_input_report_timer_start_options_t options) {
    (void)context;
    (void)adapter;
    (void)options;
    return 0;
}

static int fake_report_timer_on_can_send_now(void *context,
                                             swbt_btstack_input_report_timer_adapter_t *adapter) {
    (void)context;
    (void)adapter;
    return 0;
}

static int fake_report_timer_enqueue_reply(void *context,
                                           swbt_btstack_input_report_timer_adapter_t *adapter,
                                           uint16_t hid_cid, const uint8_t *report,
                                           size_t report_size) {
    fake_ops_t *fake = context;
    (void)adapter;
    fake->enqueue_reply_calls += 1;
    fake->last_reply_hid_cid = hid_cid;
    fake->last_reply_size = report_size;
    if (report_size <= sizeof(fake->last_reply)) {
        // The fake capture buffer capacity is checked before copying reply bytes.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        (void)memcpy(fake->last_reply, report, report_size);
    }
    return 0;
}

static int fake_report_timer_send_neutral_now(void *context,
                                              swbt_btstack_input_report_timer_adapter_t *adapter) {
    fake_ops_t *fake = context;
    (void)adapter;
    fake->timer_send_neutral_now_calls += 1;
    return fake->timer_send_neutral_now_result;
}

static void fake_report_timer_stop(void *context,
                                   swbt_btstack_input_report_timer_adapter_t *adapter) {
    (void)context;
    (void)adapter;
}

static swbt_state_t fake_state_provider(void *context) {
    (void)context;
    return swbt_state_neutral();
}

static swbt_btstack_device_port_t fake_device_port(void) {
    return (swbt_btstack_device_port_t){
        .platform_start = fake_device_platform_start,
        .platform_stop = fake_device_platform_stop,
        .hid_register = fake_hid_register,
        .hid_stop = fake_hid_stop,
        .connect = fake_device_connect,
        .send = fake_device_send,
    };
}

static swbt_btstack_production_report_timer_port_t fake_report_timer_port(void) {
    return (swbt_btstack_production_report_timer_port_t){
        .init = fake_report_timer_init,
        .start = fake_report_timer_start,
        .on_can_send_now = fake_report_timer_on_can_send_now,
        .enqueue_subcommand_reply = fake_report_timer_enqueue_reply,
        .send_neutral_now = fake_report_timer_send_neutral_now,
        .stop = fake_report_timer_stop,
    };
}

static int btstack_report_timer_bridge_sender_uses_device_send(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    swbt_btstack_device_t device;
    swbt_btstack_input_report_timer_adapter_t adapter = {0};
    swbt_daemon_process_t *host = NULL;
    bool initialized = false;
    uint8_t service_buffer[16];
    const swbt_btstack_device_port_t device_port = fake_device_port();
    const swbt_btstack_production_report_timer_port_t report_timer_port = fake_report_timer_port();
    const swbt_btstack_hid_registration_config_t registration = {0};
    const uint8_t message[] = {0xa1u, 0x30u, 0x01u, 0x02u};
    swbt_daemon_btstack_report_timer_bridge_t timer;

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_device_init(&device, &device_port, &fake),
                            SWBT_BTSTACK_DEVICE_OK, "device init");
    failed +=
        expect_eq_int(swbt_btstack_device_open(&device,
                                               (swbt_btstack_device_open_options_t){
                                                   .service_buffer = service_buffer,
                                                   .service_buffer_size = sizeof(service_buffer),
                                                   .registration = &registration,
                                               }),
                      SWBT_BTSTACK_DEVICE_OK, "device open");

    timer = (swbt_daemon_btstack_report_timer_bridge_t){
        .config = &config,
        .port = &report_timer_port,
        .port_context = &fake,
        .adapter = &adapter,
        .device = &device,
        .host = &host,
        .initialized = &initialized,
    };

    failed += expect_eq_int(
        swbt_daemon_btstack_report_timer_bridge_start(&timer, fake_state_provider, NULL), 0,
        "timer start");
    failed += expect_true(initialized, "timer initialized");
    failed += expect_eq_int(fake.report_timer_init_calls, 1, "timer init calls");
    failed += expect_true(fake.captured_timer_config.hid_sender != NULL, "hid sender configured");
    if (fake.captured_timer_config.hid_sender != NULL) {
        failed += expect_eq_int(
            fake.captured_timer_config.hid_sender(fake.captured_timer_config.hid_sender_context,
                                                  0x0042u, message, sizeof(message)),
            0, "hid sender result");
    }
    failed += expect_eq_int(fake.device_send_calls, 1, "device send calls");
    failed += expect_eq_u16(fake.last_device_hid_cid, 0x0042u, "device send cid");
    failed += expect_eq_int((int)fake.last_device_message_size, (int)sizeof(message),
                            "device send message size");
    for (size_t index = 0; index < sizeof(message); ++index) {
        failed += expect_eq_u8(fake.last_device_message[index], message[index],
                               "device send message byte");
    }

    swbt_btstack_device_close(&device);
    return failed;
}

static int report_tick_observer_updates_metrics(report_tick_metrics_case_t test_case) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    swbt_btstack_device_t device = {0};
    swbt_btstack_input_report_timer_adapter_t adapter = {0};
    swbt_daemon_process_t host;
    swbt_daemon_process_t *host_ref = &host;
    bool initialized = false;
    const swbt_btstack_production_report_timer_port_t report_timer_port = fake_report_timer_port();
    swbt_daemon_btstack_report_timer_bridge_t timer = {
        .config = &config,
        .port = &report_timer_port,
        .port_context = &fake,
        .adapter = &adapter,
        .device = &device,
        .host = &host_ref,
        .initialized = &initialized,
    };
    swbt_domain_status_snapshot_t status;

    int failed = 0;
    failed += expect_eq_int(
        swbt_daemon_process_init(&host, &config, swbt_daemon_process_noop_backend(), NULL),
        SWBT_DAEMON_PROCESS_OK, "host init");
    failed += expect_eq_int(
        swbt_daemon_btstack_report_timer_bridge_start(&timer, fake_state_provider, NULL), 0,
        "timer start");
    failed += expect_true(fake.captured_timer_config.report_tick_observer != NULL,
                          "report tick observer configured");
    if (fake.captured_timer_config.report_tick_observer != NULL) {
        fake.captured_timer_config.report_tick_observer(
            fake.captured_timer_config.report_tick_context, 123000u, test_case.send_result);
    }
    failed += expect_eq_int(swbt_domain_read_status(swbt_daemon_process_app(&host), &status),
                            SWBT_DOMAIN_OK, "status read");
    failed += expect_eq_int((int)status.metrics.report_ticks, 1, "report ticks");
    failed +=
        expect_eq_int((int)status.metrics.report_send_ok, test_case.expected_ok, "report send ok");
    failed += expect_eq_int((int)status.metrics.report_send_failed, test_case.expected_failed,
                            "report send failed");
    failed += expect_eq_int((int)status.metrics.hardware_status,
                            (int)SWBT_METRICS_HARDWARE_UNAVAILABLE, "hardware metrics");

    swbt_daemon_process_destroy(&host);
    return failed;
}

static int successful_report_tick_updates_metrics(void) {
    return report_tick_observer_updates_metrics((report_tick_metrics_case_t){
        .send_result = SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_OK,
        .expected_ok = 1,
        .expected_failed = 0,
    });
}

static int failed_report_tick_updates_metrics(void) {
    return report_tick_observer_updates_metrics((report_tick_metrics_case_t){
        .send_result = SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_FAILED,
        .expected_ok = 0,
        .expected_failed = 1,
    });
}

static int neutral_send_returns_port_result(int port_result) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .timer_send_neutral_now_result = port_result,
    };
    swbt_btstack_device_t device = {0};
    swbt_btstack_input_report_timer_adapter_t adapter = {0};
    swbt_daemon_process_t *host_ref = NULL;
    bool initialized = false;
    const swbt_btstack_production_report_timer_port_t report_timer_port = fake_report_timer_port();
    swbt_daemon_btstack_report_timer_bridge_t timer = {
        .config = &config,
        .port = &report_timer_port,
        .port_context = &fake,
        .adapter = &adapter,
        .device = &device,
        .host = &host_ref,
        .initialized = &initialized,
    };

    int failed = 0;
    failed += expect_eq_int(
        swbt_daemon_btstack_report_timer_bridge_start(&timer, fake_state_provider, NULL), 0,
        "timer start");
    adapter.running = true;
    failed += expect_eq_int(swbt_daemon_btstack_report_timer_bridge_send_neutral_now(&timer),
                            port_result, "neutral send result");
    failed += expect_eq_int(fake.timer_send_neutral_now_calls, 1, "neutral send calls");
    return failed;
}

static int neutral_send_preserves_immediate_pending_and_error_results(void) {
    int failed = 0;
    failed += neutral_send_returns_port_result(0);
    failed += neutral_send_returns_port_result(1);
    failed += neutral_send_returns_port_result(-7);
    return failed;
}

static int subcommand_reply_enqueue_routes_through_report_timer_port(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    swbt_btstack_device_t device = {0};
    swbt_btstack_input_report_timer_adapter_t adapter = {0};
    swbt_daemon_process_t *host_ref = NULL;
    bool initialized = false;
    const swbt_btstack_production_report_timer_port_t report_timer_port = fake_report_timer_port();
    const uint8_t reply[] = {0xa1u, 0x21u, 0x01u, 0x02u};
    swbt_daemon_btstack_report_timer_bridge_t timer = {
        .config = &config,
        .port = &report_timer_port,
        .port_context = &fake,
        .adapter = &adapter,
        .device = &device,
        .host = &host_ref,
        .initialized = &initialized,
    };

    int failed = 0;
    failed += expect_eq_int(
        swbt_daemon_btstack_report_timer_bridge_start(&timer, fake_state_provider, NULL), 0,
        "timer start");
    failed += expect_eq_int(swbt_daemon_btstack_report_timer_bridge_enqueue_subcommand_reply(
                                &timer, 0x0042u, reply, sizeof(reply)),
                            0, "enqueue result");
    failed += expect_eq_int(fake.enqueue_reply_calls, 1, "enqueue calls");
    failed += expect_eq_u16(fake.last_reply_hid_cid, 0x0042u, "reply hid cid");
    failed += expect_eq_int((int)fake.last_reply_size, (int)sizeof(reply), "reply size");
    for (size_t index = 0; index < sizeof(reply); ++index) {
        failed += expect_eq_u8(fake.last_reply[index], reply[index], "reply byte");
    }
    return failed;
}

int main(void) {
    int failed = 0;
    failed += btstack_report_timer_bridge_sender_uses_device_send();
    failed += successful_report_tick_updates_metrics();
    failed += failed_report_tick_updates_metrics();
    failed += neutral_send_preserves_immediate_pending_and_error_results();
    failed += subcommand_reply_enqueue_routes_through_report_timer_port();
    return failed == 0 ? 0 : 1;
}
