#ifndef SWBT_DAEMON_PROCESS_H
#define SWBT_DAEMON_PROCESS_H

#include <stdbool.h>

#include "daemon/config.h"

typedef enum {
    SWBT_DAEMON_PROCESS_OK = 0,
    SWBT_DAEMON_PROCESS_PENDING = 1,
    SWBT_DAEMON_PROCESS_ERROR_INVALID_ARGUMENT = -1,
    SWBT_DAEMON_PROCESS_ERROR_BACKEND = -2,
} swbt_daemon_process_result_t;

typedef struct swbt_daemon_process swbt_daemon_process_t;
typedef struct swbt_daemon_process_backend swbt_daemon_process_backend_t;

swbt_daemon_process_result_t swbt_daemon_process_init(swbt_daemon_process_t *host,
                                                      const swbt_daemon_config_t *config,
                                                      const swbt_daemon_process_backend_t *backend,
                                                      void *backend_context);

swbt_daemon_process_result_t swbt_daemon_process_start(swbt_daemon_process_t *host);

swbt_daemon_process_result_t swbt_daemon_process_send_neutral_now(swbt_daemon_process_t *host);

void swbt_daemon_process_stop(swbt_daemon_process_t *host);
void swbt_daemon_process_destroy(swbt_daemon_process_t *host);

bool swbt_daemon_process_is_running(const swbt_daemon_process_t *host);

const swbt_daemon_process_backend_t *swbt_daemon_process_noop_backend(void);

int swbt_daemon_main_with_process_backend(const swbt_daemon_config_t *config,
                                          const swbt_daemon_process_backend_t *backend,
                                          void *backend_context);

#endif
