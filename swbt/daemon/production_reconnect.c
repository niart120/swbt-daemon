#include "daemon/production_reconnect.h"

#include "support/diagnostics.h"
#include "daemon/switch_address.h"

static void swbt_daemon_production_reconnect_report_failed(swbt_domain_t *app) {
    const swbt_domain_hardware_status_t failed_status = {
        .adapter_state = SWBT_DOMAIN_HARDWARE_CHANNEL_UNAVAILABLE,
        .switch_connection_state = SWBT_DOMAIN_HARDWARE_CHANNEL_FAILED,
        .hid_channel_state = SWBT_DOMAIN_HARDWARE_CHANNEL_FAILED,
    };

    if (app == NULL) {
        return;
    }
    (void)swbt_domain_set_hardware_status(app, &failed_status);
}

swbt_daemon_production_reconnect_request_result_t
swbt_daemon_production_reconnect_build_request(const swbt_daemon_config_t *config,
                                               swbt_btstack_device_connect_request_t *out_request) {
    const char *switch_address = NULL;
    swbt_btstack_device_connect_request_t request = {
        .control_psm = SWBT_BTSTACK_PRODUCTION_HID_CONTROL_PSM,
        .interrupt_psm = SWBT_BTSTACK_PRODUCTION_HID_INTERRUPT_PSM,
    };

    if (config == NULL || out_request == NULL) {
        return SWBT_DAEMON_PRODUCTION_RECONNECT_REQUEST_INVALID;
    }

    switch_address = swbt_daemon_config_effective_reconnect_switch_address(config);
    if (switch_address == NULL) {
        return SWBT_DAEMON_PRODUCTION_RECONNECT_REQUEST_NONE;
    }
    if (!swbt_daemon_switch_address_parse_bytes(switch_address, request.address)) {
        return SWBT_DAEMON_PRODUCTION_RECONNECT_REQUEST_INVALID;
    }

    *out_request = request;
    return SWBT_DAEMON_PRODUCTION_RECONNECT_REQUEST_READY;
}

void swbt_daemon_production_reconnect_request_active(
    const swbt_daemon_production_reconnect_t *reconnect) {
    swbt_btstack_device_connect_request_t request;
    uint16_t hid_cid = 0u;
    int result;

    if (reconnect == NULL) {
        return;
    }

    const swbt_daemon_production_reconnect_request_result_t build_result =
        swbt_daemon_production_reconnect_build_request(reconnect->config, &request);
    if (build_result == SWBT_DAEMON_PRODUCTION_RECONNECT_REQUEST_NONE) {
        return;
    }
    if (build_result != SWBT_DAEMON_PRODUCTION_RECONNECT_REQUEST_READY) {
        swbt_diagnostic_trace("production: active reconnect address invalid");
        swbt_daemon_production_reconnect_report_failed(reconnect->app);
        return;
    }
    if (reconnect->device == NULL) {
        swbt_daemon_production_reconnect_report_failed(reconnect->app);
        return;
    }

    swbt_diagnostic_trace("production: active reconnect request");
    result = swbt_btstack_device_connect(reconnect->device, &request, &hid_cid);
    swbt_diagnostic_trace(result == 0 ? "production: active reconnect request ok"
                                      : "production: active reconnect request failed");
    if (result != 0) {
        swbt_daemon_production_reconnect_report_failed(reconnect->app);
    }
}

void swbt_daemon_production_reconnect_save_learned_address(
    swbt_daemon_config_t *config, const swbt_daemon_config_file_target_t *target,
    const uint8_t address[6]) {
    char address_text[SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE];
    swbt_daemon_config_file_result_t result;

    if (config == NULL || target == NULL || target->path == NULL || target->path[0] == '\0' ||
        address == NULL) {
        return;
    }

    swbt_daemon_switch_address_format_bytes(address, address_text);
    swbt_diagnostic_trace("production: learned switch address save");
    result = swbt_daemon_config_save_active_reconnect_learned_switch_address(config, target,
                                                                             address_text);
    swbt_diagnostic_trace(result == SWBT_DAEMON_CONFIG_FILE_OK
                              ? "production: learned switch address save ok"
                              : "production: learned switch address save failed");
}
