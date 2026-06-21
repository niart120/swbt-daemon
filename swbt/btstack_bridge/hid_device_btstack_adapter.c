#include "btstack_bridge/hid_device_btstack_adapter.h"

#include "classic/hid_device.h"
#include "classic/sdp_server.h"
#include "classic/sdp_util.h"
#include "hci.h"

static btstack_packet_callback_registration_t g_swbt_hid_hci_event_registration;

static void swbt_btstack_adapter_sdp_init(void *context) {
    (void)context;
    sdp_init();
}

static uint32_t swbt_btstack_adapter_sdp_create_service_record_handle(void *context) {
    (void)context;
    return sdp_create_service_record_handle();
}

static void
swbt_btstack_adapter_hid_create_sdp_record(void *context, uint8_t *service,
                                           uint32_t service_record_handle,
                                           const swbt_btstack_hid_sdp_record_config_t *params) {
    (void)context;
    hid_sdp_record_t record = {
        .hid_device_subclass = params->hid_device_subclass,
        .hid_country_code = params->hid_country_code,
        .hid_virtual_cable = params->hid_virtual_cable,
        .hid_remote_wake = params->hid_remote_wake,
        .hid_reconnect_initiate = params->hid_reconnect_initiate,
        .hid_normally_connectable = params->hid_normally_connectable,
        .hid_boot_device = params->hid_boot_device,
        .hid_ssr_host_max_latency = params->hid_ssr_host_max_latency,
        .hid_ssr_host_min_timeout = params->hid_ssr_host_min_timeout,
        .hid_supervision_timeout = params->hid_supervision_timeout,
        .hid_descriptor = params->hid_descriptor,
        .hid_descriptor_size = params->hid_descriptor_size,
        .device_name = params->device_name,
    };
    hid_create_sdp_record(service, service_record_handle, &record);
}

static size_t swbt_btstack_adapter_sdp_record_len(void *context, const uint8_t *service) {
    (void)context;
    return (size_t)de_get_len(service);
}

static uint8_t swbt_btstack_adapter_sdp_register_service(void *context, const uint8_t *service) {
    (void)context;
    return sdp_register_service(service);
}

static void swbt_btstack_adapter_hid_device_init(void *context, bool boot_protocol_mode_supported,
                                                 uint16_t hid_descriptor_len,
                                                 const uint8_t *hid_descriptor) {
    (void)context;
    hid_device_init(boot_protocol_mode_supported, hid_descriptor_len, hid_descriptor);
    hid_device_accept_truncated_hid_reports(true);
}

static void
swbt_btstack_adapter_hid_device_register_packet_handler(void *context,
                                                        swbt_btstack_packet_handler_t handler) {
    (void)context;
    g_swbt_hid_hci_event_registration.callback = handler;
    hci_add_event_handler(&g_swbt_hid_hci_event_registration);
    hid_device_register_packet_handler(handler);
}

const swbt_btstack_hid_registration_backend_t *swbt_btstack_hid_registration_backend_btstack(void) {
    static const swbt_btstack_hid_registration_backend_t backend = {
        .sdp_init = swbt_btstack_adapter_sdp_init,
        .sdp_create_service_record_handle = swbt_btstack_adapter_sdp_create_service_record_handle,
        .hid_create_sdp_record = swbt_btstack_adapter_hid_create_sdp_record,
        .sdp_record_len = swbt_btstack_adapter_sdp_record_len,
        .sdp_register_service = swbt_btstack_adapter_sdp_register_service,
        .hid_device_init = swbt_btstack_adapter_hid_device_init,
        .hid_device_register_packet_handler =
            swbt_btstack_adapter_hid_device_register_packet_handler,
    };
    return &backend;
}
