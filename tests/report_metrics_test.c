#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "core/logging.h"
#include "core/metrics.h"

typedef struct {
    swbt_log_event_t events[4];
    int count;
} memory_log_sink_t;

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u64(uint64_t actual, uint64_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_hardware_status(swbt_metrics_hardware_status_t actual,
                                     swbt_metrics_hardware_status_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_log_event_type(swbt_log_event_type_t actual, swbt_log_event_type_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_str(const char *actual, const char *expected) {
    return strcmp(actual, expected) == 0 ? 0 : 1;
}

static void memory_log_sink(void *context, const swbt_log_event_t *event) {
    memory_log_sink_t *sink = context;
    if (sink->count >= 0 && sink->count < 4) {
        sink->events[sink->count] = *event;
    }
    sink->count += 1;
}

static int metrics_init_returns_zero_and_unavailable_hardware_fields(void) {
    swbt_metrics_t metrics;
    swbt_metrics_snapshot_t snapshot;

    int failed = 0;
    failed += expect_eq_int(swbt_metrics_init(&metrics), SWBT_METRICS_OK);
    failed += expect_eq_int(swbt_metrics_snapshot(&metrics, &snapshot), SWBT_METRICS_OK);
    failed += expect_eq_u64(snapshot.report_ticks, 0u);
    failed += expect_eq_u64(snapshot.report_send_ok, 0u);
    failed += expect_eq_u64(snapshot.report_send_failed, 0u);
    failed += expect_eq_u64(snapshot.report_interval_average_us, 0u);
    failed += expect_eq_u64(snapshot.report_interval_max_us, 0u);
    failed +=
        expect_eq_hardware_status(snapshot.hardware_status, SWBT_METRICS_HARDWARE_UNAVAILABLE);
    failed += expect_eq_u32(snapshot.actual_report_rate_hz, 0u);
    failed += expect_eq_u64(snapshot.jitter_max_us, 0u);
    return failed;
}

static int report_ticks_update_intervals_and_send_counters(void) {
    swbt_metrics_t metrics;
    swbt_metrics_snapshot_t snapshot;

    int failed = 0;
    failed += expect_eq_int(swbt_metrics_init(&metrics), SWBT_METRICS_OK);
    failed +=
        expect_eq_int(swbt_metrics_record_report_tick(&metrics, 1000u, SWBT_METRICS_REPORT_SEND_OK),
                      SWBT_METRICS_OK);
    failed +=
        expect_eq_int(swbt_metrics_record_report_tick(&metrics, 9000u, SWBT_METRICS_REPORT_SEND_OK),
                      SWBT_METRICS_OK);
    failed += expect_eq_int(
        swbt_metrics_record_report_tick(&metrics, 22000u, SWBT_METRICS_REPORT_SEND_FAILED),
        SWBT_METRICS_OK);
    failed += expect_eq_int(swbt_metrics_snapshot(&metrics, &snapshot), SWBT_METRICS_OK);
    failed += expect_eq_u64(snapshot.report_ticks, 3u);
    failed += expect_eq_u64(snapshot.report_send_ok, 2u);
    failed += expect_eq_u64(snapshot.report_send_failed, 1u);
    failed += expect_eq_u64(snapshot.report_interval_count, 2u);
    failed += expect_eq_u64(snapshot.report_interval_average_us, 10500u);
    failed += expect_eq_u64(snapshot.report_interval_max_us, 13000u);
    return failed;
}

static int state_update_events_record_accepted_rejected_and_coalesced_counts(void) {
    swbt_metrics_t metrics;
    swbt_metrics_snapshot_t snapshot;

    int failed = 0;
    failed += expect_eq_int(swbt_metrics_init(&metrics), SWBT_METRICS_OK);
    failed +=
        expect_eq_int(swbt_metrics_record_state_update_accepted(&metrics, 2u), SWBT_METRICS_OK);
    failed +=
        expect_eq_int(swbt_metrics_record_state_update_accepted(&metrics, 0u), SWBT_METRICS_OK);
    failed += expect_eq_int(swbt_metrics_record_state_update_rejected(&metrics), SWBT_METRICS_OK);
    failed += expect_eq_int(swbt_metrics_snapshot(&metrics, &snapshot), SWBT_METRICS_OK);
    failed += expect_eq_u64(snapshot.ipc_state_accepted, 2u);
    failed += expect_eq_u64(snapshot.ipc_state_rejected, 1u);
    failed += expect_eq_u64(snapshot.ipc_state_coalesced, 2u);
    return failed;
}

static int startup_and_shutdown_logs_emit_configured_fields_without_hardware_claims(void) {
    swbt_logger_t logger;
    memory_log_sink_t sink = {0};

    int failed = 0;
    failed += expect_eq_int(swbt_logger_init(&logger, memory_log_sink, &sink), SWBT_LOG_OK);
    failed += expect_eq_int(
        swbt_log_daemon_startup(&logger, 8000u, SWBT_METRICS_HARDWARE_UNAVAILABLE), SWBT_LOG_OK);
    failed += expect_eq_int(swbt_log_daemon_shutdown(&logger, 0), SWBT_LOG_OK);
    failed += expect_eq_int(sink.count, 2);
    failed += expect_eq_log_event_type(sink.events[0].type, SWBT_LOG_EVENT_DAEMON_STARTUP);
    failed += expect_eq_hardware_status(sink.events[0].hardware_status,
                                        SWBT_METRICS_HARDWARE_UNAVAILABLE);
    failed += expect_eq_u32(sink.events[0].report_period_us, 8000u);
    failed += expect_eq_str(sink.events[0].component, "daemon");
    failed += expect_eq_log_event_type(sink.events[1].type, SWBT_LOG_EVENT_DAEMON_SHUTDOWN);
    failed += expect_eq_int(sink.events[1].exit_code, 0);
    return failed;
}

static int metrics_snapshot_does_not_mutate_live_counters(void) {
    swbt_metrics_t metrics;
    swbt_metrics_snapshot_t first;
    swbt_metrics_snapshot_t second;
    swbt_metrics_snapshot_t third;

    int failed = 0;
    failed += expect_eq_int(swbt_metrics_init(&metrics), SWBT_METRICS_OK);
    failed +=
        expect_eq_int(swbt_metrics_record_report_tick(&metrics, 1000u, SWBT_METRICS_REPORT_SEND_OK),
                      SWBT_METRICS_OK);
    failed +=
        expect_eq_int(swbt_metrics_record_report_tick(&metrics, 9000u, SWBT_METRICS_REPORT_SEND_OK),
                      SWBT_METRICS_OK);
    failed += expect_eq_int(swbt_metrics_snapshot(&metrics, &first), SWBT_METRICS_OK);
    failed += expect_eq_int(swbt_metrics_snapshot(&metrics, &second), SWBT_METRICS_OK);
    failed += expect_true(first.report_ticks == second.report_ticks);
    failed += expect_true(first.report_interval_count == second.report_interval_count);

    failed += expect_eq_int(
        swbt_metrics_record_report_tick(&metrics, 17000u, SWBT_METRICS_REPORT_SEND_OK),
        SWBT_METRICS_OK);
    failed += expect_eq_int(swbt_metrics_snapshot(&metrics, &third), SWBT_METRICS_OK);
    failed += expect_eq_u64(third.report_ticks, 3u);
    failed += expect_eq_u64(third.report_interval_count, 2u);
    failed += expect_eq_u64(third.report_interval_average_us, 8000u);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += metrics_init_returns_zero_and_unavailable_hardware_fields();
    failed += report_ticks_update_intervals_and_send_counters();
    failed += state_update_events_record_accepted_rejected_and_coalesced_counts();
    failed += startup_and_shutdown_logs_emit_configured_fields_without_hardware_claims();
    failed += metrics_snapshot_does_not_mutate_live_counters();
    return failed == 0 ? 0 : 1;
}
