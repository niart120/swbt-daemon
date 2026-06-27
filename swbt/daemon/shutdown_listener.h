#ifndef SWBT_DAEMON_SHUTDOWN_LISTENER_H
#define SWBT_DAEMON_SHUTDOWN_LISTENER_H

typedef void (*swbt_daemon_shutdown_request_t)(void *context);

typedef struct {
    int (*install)(void *context, swbt_daemon_shutdown_request_t request_shutdown,
                   void *request_context);
    void (*uninstall)(void *context);
} swbt_daemon_shutdown_listener_t;

#endif
