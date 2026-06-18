#include "btstack_bridge/hid_device_registration.h"

static bool
swbt_btstack_hid_backend_is_valid(const swbt_btstack_hid_registration_backend_t *backend) {
    return backend != NULL && backend->sdp_init != NULL &&
           backend->sdp_create_service_record_handle != NULL &&
           backend->hid_create_sdp_record != NULL && backend->sdp_record_len != NULL &&
           backend->sdp_register_service != NULL && backend->hid_device_init != NULL &&
           backend->hid_device_register_packet_handler != NULL;
}

static bool swbt_btstack_hid_config_is_valid(const swbt_btstack_hid_registration_config_t *config) {
    return config != NULL && config->hid_descriptor != NULL && config->hid_descriptor_size > 0u &&
           config->packet_handler != NULL;
}

static swbt_btstack_hid_sdp_record_config_t
swbt_btstack_hid_make_sdp_record_config(const swbt_btstack_hid_registration_config_t *config) {
    swbt_btstack_hid_sdp_record_config_t record = {
        .hid_device_subclass = config->hid_device_subclass,
        .hid_country_code = config->hid_country_code,
        .hid_virtual_cable = config->hid_virtual_cable,
        .hid_remote_wake = config->hid_remote_wake,
        .hid_reconnect_initiate = config->hid_reconnect_initiate,
        .hid_normally_connectable = config->hid_normally_connectable,
        .hid_boot_device = config->hid_boot_device,
        .hid_ssr_host_max_latency = config->hid_ssr_host_max_latency,
        .hid_ssr_host_min_timeout = config->hid_ssr_host_min_timeout,
        .hid_supervision_timeout = config->hid_supervision_timeout,
        .hid_descriptor = config->hid_descriptor,
        .hid_descriptor_size = config->hid_descriptor_size,
        .device_name = config->device_name,
    };
    return record;
}

swbt_btstack_hid_registration_result_t
swbt_btstack_hid_device_register(const swbt_btstack_hid_registration_backend_t *backend,
                                 void *backend_context, uint8_t *service_buffer,
                                 size_t service_buffer_size,
                                 const swbt_btstack_hid_registration_config_t *config) {
    if (!swbt_btstack_hid_backend_is_valid(backend) || !swbt_btstack_hid_config_is_valid(config) ||
        service_buffer == NULL || service_buffer_size == 0u) {
        return SWBT_BTSTACK_HID_REGISTRATION_ERROR_INVALID_ARGUMENT;
    }

    for (size_t index = 0; index < service_buffer_size; ++index) {
        service_buffer[index] = 0;
    }

    backend->sdp_init(backend_context);
    const uint32_t service_record_handle =
        backend->sdp_create_service_record_handle(backend_context);
    const swbt_btstack_hid_sdp_record_config_t sdp_record_config =
        swbt_btstack_hid_make_sdp_record_config(config);

    backend->hid_create_sdp_record(backend_context, service_buffer, service_record_handle,
                                   &sdp_record_config);

    const size_t record_len = backend->sdp_record_len(backend_context, service_buffer);
    if (record_len > service_buffer_size) {
        return SWBT_BTSTACK_HID_REGISTRATION_ERROR_SDP_RECORD_TOO_LARGE;
    }

    if (backend->sdp_register_service(backend_context, service_buffer) != 0u) {
        return SWBT_BTSTACK_HID_REGISTRATION_ERROR_SDP_REGISTER_FAILED;
    }

    backend->hid_device_init(backend_context, config->hid_boot_device, config->hid_descriptor_size,
                             config->hid_descriptor);
    backend->hid_device_register_packet_handler(backend_context, config->packet_handler);

    return SWBT_BTSTACK_HID_REGISTRATION_OK;
}
