#ifndef SWBT_SUPPORT_LOGGING_H
#define SWBT_SUPPORT_LOGGING_H

#include <stdint.h>

#include "support/metrics.h"

typedef enum {
    SWBT_LOG_OK = 0,
    SWBT_LOG_ERROR_INVALID_ARGUMENT = -1,
} swbt_log_result_t;

typedef enum {
    SWBT_LOG_LEVEL_INFO = 0,
    SWBT_LOG_LEVEL_ERROR = 1,
} swbt_log_level_t;

typedef enum {
    SWBT_LOG_EVENT_NONE = 0,
    SWBT_LOG_EVENT_DAEMON_STARTUP = 1,
    SWBT_LOG_EVENT_DAEMON_SHUTDOWN = 2,
} swbt_log_event_type_t;

typedef struct {
    swbt_log_event_type_t type;
    swbt_log_level_t level;
    const char *component;
    const char *message;
    uint32_t report_period_us;
    swbt_metrics_hardware_status_t hardware_status;
    int exit_code;
} swbt_log_event_t;

typedef void (*swbt_log_sink_t)(void *context, const swbt_log_event_t *event);

typedef struct {
    swbt_log_sink_t sink;
    void *context;
} swbt_logger_t;

swbt_log_result_t swbt_logger_init(swbt_logger_t *logger, swbt_log_sink_t sink, void *context);

swbt_log_result_t swbt_log_daemon_startup(swbt_logger_t *logger, uint32_t report_period_us,
                                          swbt_metrics_hardware_status_t hardware_status);

swbt_log_result_t swbt_log_daemon_shutdown(swbt_logger_t *logger, int exit_code);

#endif
