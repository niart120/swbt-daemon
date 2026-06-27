#include "support/logging.h"

#include <stddef.h>

static swbt_log_result_t swbt_log_emit(swbt_logger_t *logger, const swbt_log_event_t *event) {
    if (logger == NULL || logger->sink == NULL || event == NULL) {
        return SWBT_LOG_ERROR_INVALID_ARGUMENT;
    }

    logger->sink(logger->context, event);
    return SWBT_LOG_OK;
}

swbt_log_result_t swbt_logger_init(swbt_logger_t *logger, swbt_log_sink_t sink, void *context) {
    if (logger == NULL || sink == NULL) {
        return SWBT_LOG_ERROR_INVALID_ARGUMENT;
    }

    logger->sink = sink;
    logger->context = context;
    return SWBT_LOG_OK;
}

swbt_log_result_t swbt_log_daemon_startup(swbt_logger_t *logger, uint32_t report_period_us,
                                          swbt_metrics_hardware_status_t hardware_status) {
    swbt_log_event_t event = {
        .type = SWBT_LOG_EVENT_DAEMON_STARTUP,
        .level = SWBT_LOG_LEVEL_INFO,
        .component = "daemon",
        .message = "startup",
        .report_period_us = report_period_us,
        .hardware_status = hardware_status,
        .exit_code = 0,
    };
    return swbt_log_emit(logger, &event);
}

swbt_log_result_t swbt_log_daemon_shutdown(swbt_logger_t *logger, int exit_code) {
    swbt_log_event_t event = {
        .type = SWBT_LOG_EVENT_DAEMON_SHUTDOWN,
        .level = SWBT_LOG_LEVEL_INFO,
        .component = "daemon",
        .message = "shutdown",
        .report_period_us = 0u,
        .hardware_status = SWBT_METRICS_HARDWARE_UNAVAILABLE,
        .exit_code = exit_code,
    };
    return swbt_log_emit(logger, &event);
}
