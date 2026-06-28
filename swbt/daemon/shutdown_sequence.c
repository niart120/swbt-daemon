#include "daemon/shutdown_sequence.h"

#include "support/diagnostics.h"

static bool
swbt_daemon_shutdown_sequence_is_ready(const swbt_daemon_shutdown_sequence_t *shutdown) {
    return shutdown != NULL && shutdown->run_loop != NULL &&
           shutdown->run_loop->execute_on_main_thread != NULL && shutdown->finish != NULL;
}

bool swbt_daemon_shutdown_sequence_listener_is_valid(
    const swbt_daemon_shutdown_listener_t *shutdown_listener) {
    return shutdown_listener == NULL ||
           (shutdown_listener->install != NULL && shutdown_listener->uninstall != NULL);
}

bool swbt_daemon_shutdown_sequence_init(swbt_daemon_shutdown_sequence_t *shutdown,
                                        const swbt_daemon_shutdown_sequence_config_t *config) {
    if (shutdown == NULL || config == NULL || config->run_loop == NULL ||
        config->run_loop->execute_on_main_thread == NULL || config->finish == NULL) {
        return false;
    }

    shutdown->run_loop = config->run_loop;
    shutdown->port_context = config->port_context;
    shutdown->host = config->host;
    shutdown->finish = config->finish;
    shutdown->finish_context = config->finish_context;
    shutdown->neutral_pending = false;
    shutdown->disconnect_pending = false;
    shutdown->callback = (btstack_context_callback_registration_t){0};
    atomic_init(&shutdown->requested, false);
    return true;
}

void swbt_daemon_shutdown_sequence_prepare(swbt_daemon_shutdown_sequence_t *shutdown) {
    if (shutdown == NULL) {
        return;
    }

    atomic_store(&shutdown->requested, false);
    shutdown->neutral_pending = false;
    shutdown->disconnect_pending = false;
    shutdown->callback = (btstack_context_callback_registration_t){0};
}

void swbt_daemon_shutdown_sequence_finish(swbt_daemon_shutdown_sequence_t *shutdown) {
    if (!swbt_daemon_shutdown_sequence_is_ready(shutdown)) {
        return;
    }

    shutdown->finish(shutdown->finish_context);
}

static void swbt_daemon_shutdown_sequence_on_main_thread(void *context) {
    swbt_daemon_shutdown_sequence_t *shutdown = context;
    if (!swbt_daemon_shutdown_sequence_is_ready(shutdown)) {
        return;
    }

    if (shutdown->host != NULL && *shutdown->host != NULL) {
        swbt_diagnostic_trace("production: shutdown neutral send");
        const swbt_daemon_process_result_t neutral_result =
            swbt_daemon_process_send_neutral_now(*shutdown->host);
        if (neutral_result == SWBT_DAEMON_PROCESS_OK) {
            swbt_diagnostic_trace("production: shutdown neutral send ok");
            swbt_daemon_shutdown_sequence_finish(shutdown);
        } else if (neutral_result == SWBT_DAEMON_PROCESS_PENDING) {
            swbt_diagnostic_trace("production: shutdown neutral send pending");
            shutdown->neutral_pending = true;
        } else {
            swbt_diagnostic_trace("production: shutdown neutral send failed");
            swbt_daemon_shutdown_sequence_finish(shutdown);
        }
    } else {
        swbt_daemon_shutdown_sequence_finish(shutdown);
    }
}

void swbt_daemon_shutdown_sequence_request(void *context) {
    swbt_daemon_shutdown_sequence_t *shutdown = context;
    if (!swbt_daemon_shutdown_sequence_is_ready(shutdown) ||
        atomic_exchange(&shutdown->requested, true)) {
        return;
    }

    swbt_diagnostic_trace("production: shutdown requested");
    shutdown->callback = (btstack_context_callback_registration_t){
        .callback = swbt_daemon_shutdown_sequence_on_main_thread,
        .context = shutdown,
    };
    shutdown->run_loop->execute_on_main_thread(shutdown->port_context, &shutdown->callback);
}

int swbt_daemon_shutdown_sequence_install_listener(
    swbt_daemon_shutdown_sequence_t *shutdown,
    const swbt_daemon_shutdown_listener_t *shutdown_listener, void *shutdown_context) {
    if (!swbt_daemon_shutdown_sequence_is_ready(shutdown) ||
        !swbt_daemon_shutdown_sequence_listener_is_valid(shutdown_listener)) {
        return -1;
    }
    if (shutdown_listener == NULL) {
        return 0;
    }

    return shutdown_listener->install(shutdown_context, swbt_daemon_shutdown_sequence_request,
                                      shutdown);
}

void swbt_daemon_shutdown_sequence_uninstall_listener(
    const swbt_daemon_shutdown_listener_t *shutdown_listener, void *shutdown_context) {
    if (swbt_daemon_shutdown_sequence_listener_is_valid(shutdown_listener) &&
        shutdown_listener != NULL) {
        shutdown_listener->uninstall(shutdown_context);
    }
}
