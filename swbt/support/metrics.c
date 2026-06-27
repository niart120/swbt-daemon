#include "support/metrics.h"

#include <stddef.h>

static bool swbt_metrics_send_result_is_valid(swbt_metrics_report_send_result_t send_result) {
    return send_result == SWBT_METRICS_REPORT_SEND_OK ||
           send_result == SWBT_METRICS_REPORT_SEND_FAILED;
}

swbt_metrics_result_t swbt_metrics_init(swbt_metrics_t *metrics) {
    if (metrics == NULL) {
        return SWBT_METRICS_ERROR_INVALID_ARGUMENT;
    }

    *metrics = (swbt_metrics_t){0};
    metrics->snapshot.hardware_status = SWBT_METRICS_HARDWARE_UNAVAILABLE;
    return SWBT_METRICS_OK;
}

swbt_metrics_result_t swbt_metrics_record_report_tick(swbt_metrics_t *metrics,
                                                      swbt_metrics_report_tick_options_t options) {
    const uint64_t now_us = options.now_us;
    const swbt_metrics_report_send_result_t send_result = options.send_result;

    if (metrics == NULL || !swbt_metrics_send_result_is_valid(send_result)) {
        return SWBT_METRICS_ERROR_INVALID_ARGUMENT;
    }
    if (metrics->has_last_report_tick && now_us < metrics->last_report_tick_us) {
        return SWBT_METRICS_ERROR_INVALID_ARGUMENT;
    }

    metrics->snapshot.report_ticks += 1u;
    if (send_result == SWBT_METRICS_REPORT_SEND_OK) {
        metrics->snapshot.report_send_ok += 1u;
    } else {
        metrics->snapshot.report_send_failed += 1u;
    }

    if (metrics->has_last_report_tick) {
        const uint64_t interval_us = now_us - metrics->last_report_tick_us;
        metrics->snapshot.report_interval_count += 1u;
        metrics->report_interval_total_us += interval_us;
        metrics->snapshot.report_interval_average_us =
            metrics->report_interval_total_us / metrics->snapshot.report_interval_count;
        if (interval_us > metrics->snapshot.report_interval_max_us) {
            metrics->snapshot.report_interval_max_us = interval_us;
        }
    }

    metrics->last_report_tick_us = now_us;
    metrics->has_last_report_tick = true;
    return SWBT_METRICS_OK;
}

swbt_metrics_result_t swbt_metrics_record_state_update_accepted(swbt_metrics_t *metrics,
                                                                uint64_t coalesced_updates) {
    if (metrics == NULL) {
        return SWBT_METRICS_ERROR_INVALID_ARGUMENT;
    }

    metrics->snapshot.ipc_state_accepted += 1u;
    metrics->snapshot.ipc_state_coalesced += coalesced_updates;
    return SWBT_METRICS_OK;
}

swbt_metrics_result_t swbt_metrics_record_state_update_rejected(swbt_metrics_t *metrics) {
    if (metrics == NULL) {
        return SWBT_METRICS_ERROR_INVALID_ARGUMENT;
    }

    metrics->snapshot.ipc_state_rejected += 1u;
    return SWBT_METRICS_OK;
}

swbt_metrics_result_t swbt_metrics_snapshot(const swbt_metrics_t *metrics,
                                            swbt_metrics_snapshot_t *out_snapshot) {
    if (metrics == NULL || out_snapshot == NULL) {
        return SWBT_METRICS_ERROR_INVALID_ARGUMENT;
    }

    *out_snapshot = metrics->snapshot;
    return SWBT_METRICS_OK;
}
