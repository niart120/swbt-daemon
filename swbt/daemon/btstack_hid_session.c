#include "daemon/btstack_hid_session.h"

#include "support/diagnostics.h"
#include "btstack_bridge/hid_event.h"
#include "daemon/active_reconnect.h"

static swbt_daemon_btstack_hid_session_t *g_active_session;

static bool swbt_daemon_btstack_hid_session_register_is_valid(
    const swbt_daemon_btstack_hid_session_t *session) {
    return session != NULL && session->device_port != NULL && session->device != NULL &&
           session->service_buffer != NULL && session->service_buffer_size > 0u;
}

static bool swbt_daemon_btstack_hid_session_report_timer_is_ready(
    const swbt_daemon_btstack_hid_session_t *session) {
    return session != NULL && session->report_timer_port != NULL &&
           session->report_timer_port->start != NULL &&
           session->report_timer_port->on_can_send_now != NULL &&
           session->report_timer_port->stop != NULL && session->clock_port != NULL &&
           session->clock_port->time_ms != NULL && session->report_timer != NULL &&
           session->report_timer_initialized != NULL && *session->report_timer_initialized;
}

static void
swbt_daemon_btstack_hid_session_finish_shutdown(const swbt_daemon_btstack_hid_session_t *session) {
    if (session != NULL && session->finish_shutdown != NULL) {
        session->finish_shutdown(session->finish_shutdown_context);
    }
}

static void swbt_daemon_btstack_hid_session_maybe_save_learned_address(
    swbt_daemon_btstack_hid_session_t *session, const uint8_t address[6]) {
    if (session->config != NULL && session->learned_switch_address_target != NULL &&
        session->learned_switch_address_target_configured != NULL &&
        *session->learned_switch_address_target_configured) {
        swbt_daemon_active_reconnect_save_learned_address(
            session->config, session->learned_switch_address_target, address);
    }
}

static void
swbt_daemon_btstack_hid_session_handle_user_confirmation(swbt_daemon_btstack_hid_session_t *session,
                                                         const uint8_t address[6]) {
    if (session->controller_port == NULL ||
        session->controller_port->confirm_ssp_user_confirmation == NULL) {
        return;
    }

    (void)session->controller_port->confirm_ssp_user_confirmation(session->port_context, address);
}

static void
swbt_daemon_btstack_hid_session_handle_connection_opened(swbt_daemon_btstack_hid_session_t *session,
                                                         const swbt_btstack_hid_event_t *event) {
    if (!swbt_daemon_btstack_hid_session_report_timer_is_ready(session) || event->status != 0u) {
        return;
    }

    swbt_diagnostic_trace("production: hid connection opened");
    swbt_daemon_btstack_hid_session_maybe_save_learned_address(session, event->address);
    (void)session->report_timer_port->start(
        session->port_context, session->report_timer,
        (swbt_btstack_input_report_timer_start_options_t){
            .hid_cid = event->hid_cid,
            .now_us = (uint64_t)session->clock_port->time_ms(session->port_context) * 1000u,
        });
}

static void
swbt_daemon_btstack_hid_session_handle_can_send_now(swbt_daemon_btstack_hid_session_t *session) {
    if (!swbt_daemon_btstack_hid_session_report_timer_is_ready(session)) {
        return;
    }

    const int can_send_result =
        session->report_timer_port->on_can_send_now(session->port_context, session->report_timer);
    if (session->shutdown_neutral_pending != NULL && *session->shutdown_neutral_pending &&
        can_send_result == 0) {
        swbt_diagnostic_trace("production: shutdown neutral pending sent");
        *session->shutdown_neutral_pending = false;
        swbt_daemon_btstack_hid_session_finish_shutdown(session);
    } else if (session->shutdown_neutral_pending != NULL && *session->shutdown_neutral_pending &&
               can_send_result != 0) {
        swbt_diagnostic_trace("production: shutdown neutral pending failed");
        *session->shutdown_neutral_pending = false;
        swbt_daemon_btstack_hid_session_finish_shutdown(session);
    }
}

static void swbt_daemon_btstack_hid_session_handle_connection_closed(
    swbt_daemon_btstack_hid_session_t *session) {
    if (!swbt_daemon_btstack_hid_session_report_timer_is_ready(session)) {
        return;
    }

    swbt_diagnostic_trace("production: hid connection closed");
    session->report_timer_port->stop(session->port_context, session->report_timer);
    if (session->shutdown_neutral_pending != NULL && *session->shutdown_neutral_pending) {
        swbt_diagnostic_trace("production: shutdown neutral pending connection closed");
        *session->shutdown_neutral_pending = false;
        swbt_daemon_btstack_hid_session_finish_shutdown(session);
    }
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters): BTstack packet handler ABI.
static void swbt_daemon_btstack_hid_session_packet_handler(uint8_t packet_type, uint16_t channel,
                                                           uint8_t *packet, uint16_t size) {
    swbt_daemon_btstack_hid_session_t *session = g_active_session;
    swbt_btstack_hid_event_t event;
    (void)channel;

    if (session == NULL || swbt_btstack_device_recv(session->device, packet_type, packet, size,
                                                    &event) != SWBT_BTSTACK_DEVICE_OK) {
        return;
    }

    switch (event.type) {
    case SWBT_BTSTACK_HID_EVENT_USER_CONFIRMATION_REQUEST:
        swbt_daemon_btstack_hid_session_handle_user_confirmation(session, event.address);
        break;
    case SWBT_BTSTACK_HID_EVENT_CONNECTION_OPENED:
        swbt_daemon_btstack_hid_session_handle_connection_opened(session, &event);
        break;
    case SWBT_BTSTACK_HID_EVENT_CAN_SEND_NOW:
        swbt_daemon_btstack_hid_session_handle_can_send_now(session);
        break;
    case SWBT_BTSTACK_HID_EVENT_CONNECTION_CLOSED:
        swbt_daemon_btstack_hid_session_handle_connection_closed(session);
        break;
    case SWBT_BTSTACK_HID_EVENT_NONE:
    default:
        break;
    }
}
// NOLINTEND(bugprone-easily-swappable-parameters)

int swbt_daemon_btstack_hid_session_register(swbt_daemon_btstack_hid_session_t *session) {
    swbt_btstack_hid_registration_config_t config;
    swbt_btstack_device_result_t result;

    if (!swbt_daemon_btstack_hid_session_register_is_valid(session)) {
        return -1;
    }

    swbt_diagnostic_trace("production: hid register enter");
    config = swbt_btstack_production_hid_registration_config();
    config.packet_handler = swbt_daemon_btstack_hid_session_packet_handler;
    g_active_session = session;
    if (swbt_btstack_device_init(session->device, session->device_port, session->port_context) !=
        SWBT_BTSTACK_DEVICE_OK) {
        if (g_active_session == session) {
            g_active_session = NULL;
        }
        return -1;
    }

    swbt_diagnostic_trace("production: device open");
    result = swbt_btstack_device_open(session->device,
                                      (swbt_btstack_device_open_options_t){
                                          .service_buffer = session->service_buffer,
                                          .service_buffer_size = session->service_buffer_size,
                                          .registration = &config,
                                      });
    if (result != SWBT_BTSTACK_DEVICE_OK) {
        swbt_diagnostic_trace("production: device open failed");
        if (g_active_session == session) {
            g_active_session = NULL;
        }
        return -1;
    }

    swbt_diagnostic_trace("production: device open ok");
    return 0;
}

void swbt_daemon_btstack_hid_session_stop(swbt_daemon_btstack_hid_session_t *session) {
    if (session == NULL || session->device == NULL) {
        return;
    }
    if (g_active_session == session) {
        g_active_session = NULL;
    }
    swbt_diagnostic_trace("production: device close");
    swbt_btstack_device_close(session->device);
    swbt_diagnostic_trace("production: device close done");
}
