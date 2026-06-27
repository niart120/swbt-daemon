#ifndef SWBT_DAEMON_APP_PLATFORM_PROCESS_H
#define SWBT_DAEMON_APP_PLATFORM_PROCESS_H

#include "daemon/shutdown_listener.h"

void swbt_daemon_platform_install_crash_dump_handler(const char *path);

const swbt_daemon_shutdown_listener_t *swbt_daemon_platform_shutdown_listener(void);

#endif
