#ifndef SWBT_BTSTACK_BRIDGE_HID_DEVICE_REGISTRATION_H
#define SWBT_BTSTACK_BRIDGE_HID_DEVICE_REGISTRATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*swbt_btstack_packet_handler_t)(uint8_t packet_type, uint16_t channel,
                                              uint8_t *packet, uint16_t size);

typedef enum {
    SWBT_BTSTACK_HID_REGISTRATION_OK = 0,
    SWBT_BTSTACK_HID_REGISTRATION_ERROR_INVALID_ARGUMENT = -1,
    SWBT_BTSTACK_HID_REGISTRATION_ERROR_SDP_RECORD_TOO_LARGE = -2,
    SWBT_BTSTACK_HID_REGISTRATION_ERROR_SDP_REGISTER_FAILED = -3,
} swbt_btstack_hid_registration_result_t;

typedef struct {
    uint16_t hid_device_subclass;
    uint8_t hid_country_code;
    uint8_t hid_virtual_cable;
    uint8_t hid_remote_wake;
    uint8_t hid_reconnect_initiate;
    bool hid_normally_connectable;
    bool hid_boot_device;
    uint16_t hid_ssr_host_max_latency;
    uint16_t hid_ssr_host_min_timeout;
    uint16_t hid_supervision_timeout;
    const uint8_t *hid_descriptor;
    uint16_t hid_descriptor_size;
    const char *device_name;
} swbt_btstack_hid_sdp_record_config_t;

typedef struct {
    uint16_t hid_device_subclass;
    uint8_t hid_country_code;
    uint8_t hid_virtual_cable;
    uint8_t hid_remote_wake;
    uint8_t hid_reconnect_initiate;
    bool hid_normally_connectable;
    bool hid_boot_device;
    uint16_t hid_ssr_host_max_latency;
    uint16_t hid_ssr_host_min_timeout;
    uint16_t hid_supervision_timeout;
    const uint8_t *hid_descriptor;
    uint16_t hid_descriptor_size;
    const char *device_name;
    swbt_btstack_packet_handler_t packet_handler;
} swbt_btstack_hid_registration_config_t;

typedef struct {
    void (*sdp_init)(void *context);
    uint32_t (*sdp_create_service_record_handle)(void *context);
    void (*hid_create_sdp_record)(void *context, uint8_t *service, uint32_t service_record_handle,
                                  const swbt_btstack_hid_sdp_record_config_t *params);
    size_t (*sdp_record_len)(void *context, const uint8_t *service);
    uint8_t (*sdp_register_service)(void *context, const uint8_t *service);
    void (*hid_device_init)(void *context, bool boot_protocol_mode_supported,
                            uint16_t hid_descriptor_len, const uint8_t *hid_descriptor);
    void (*hid_device_register_packet_handler)(void *context,
                                               swbt_btstack_packet_handler_t handler);
} swbt_btstack_hid_registration_backend_t;

swbt_btstack_hid_registration_result_t
swbt_btstack_hid_device_register(const swbt_btstack_hid_registration_backend_t *backend,
                                 void *backend_context, uint8_t *service_buffer,
                                 size_t service_buffer_size,
                                 const swbt_btstack_hid_registration_config_t *config);

#endif
