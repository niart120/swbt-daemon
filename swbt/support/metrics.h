#ifndef SWBT_SUPPORT_METRICS_H
#define SWBT_SUPPORT_METRICS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SWBT_METRICS_OK = 0,
    SWBT_METRICS_ERROR_INVALID_ARGUMENT = -1,
} swbt_metrics_result_t;

typedef enum {
    SWBT_METRICS_REPORT_SEND_OK = 0,
    SWBT_METRICS_REPORT_SEND_FAILED = 1,
} swbt_metrics_report_send_result_t;

typedef enum {
    SWBT_METRICS_HARDWARE_UNAVAILABLE = 0,
    SWBT_METRICS_HARDWARE_OBSERVED = 1,
} swbt_metrics_hardware_status_t;

typedef struct {
    uint64_t report_ticks;
    uint64_t report_send_ok;
    uint64_t report_send_failed;
    uint64_t report_interval_count;
    uint64_t report_interval_average_us;
    uint64_t report_interval_max_us;
    uint64_t ipc_state_accepted;
    uint64_t ipc_state_rejected;
    uint64_t ipc_state_coalesced;
    swbt_metrics_hardware_status_t hardware_status;
    uint32_t actual_report_rate_hz;
    uint64_t jitter_max_us;
} swbt_metrics_snapshot_t;

typedef struct {
    swbt_metrics_snapshot_t snapshot;
    uint64_t report_interval_total_us;
    uint64_t last_report_tick_us;
    bool has_last_report_tick;
} swbt_metrics_t;

typedef struct {
    uint64_t now_us;
    swbt_metrics_report_send_result_t send_result;
} swbt_metrics_report_tick_options_t;

swbt_metrics_result_t swbt_metrics_init(swbt_metrics_t *metrics);

swbt_metrics_result_t swbt_metrics_record_report_tick(swbt_metrics_t *metrics,
                                                      swbt_metrics_report_tick_options_t options);

swbt_metrics_result_t swbt_metrics_record_state_update_accepted(swbt_metrics_t *metrics,
                                                                uint64_t coalesced_updates);

swbt_metrics_result_t swbt_metrics_record_state_update_rejected(swbt_metrics_t *metrics);

swbt_metrics_result_t swbt_metrics_snapshot(const swbt_metrics_t *metrics,
                                            swbt_metrics_snapshot_t *out_snapshot);

#endif
