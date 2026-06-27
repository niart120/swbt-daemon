#ifndef SWBT_BTSTACK_BRIDGE_DEVICE_H
#define SWBT_BTSTACK_BRIDGE_DEVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/hid_device_registration.h"
#include "btstack_bridge/hid_event.h"

#define SWBT_BTSTACK_DEVICE_HID_CONTROL_PSM 0x11u
#define SWBT_BTSTACK_DEVICE_HID_INTERRUPT_PSM 0x13u

typedef enum {
    SWBT_BTSTACK_DEVICE_OK = 0,
    SWBT_BTSTACK_DEVICE_IGNORED = 1,
    SWBT_BTSTACK_DEVICE_ERROR_INVALID_ARGUMENT = -1,
    SWBT_BTSTACK_DEVICE_ERROR_CLOSED = -2,
    SWBT_BTSTACK_DEVICE_ERROR_OPEN_FAILED = -3,
    SWBT_BTSTACK_DEVICE_ERROR_CONNECT_FAILED = -4,
    SWBT_BTSTACK_DEVICE_ERROR_SEND_FAILED = -5,
    SWBT_BTSTACK_DEVICE_ERROR_RECV_FAILED = -6,
} swbt_btstack_device_result_t;

typedef struct {
    uint8_t address[6];
    uint16_t control_psm;
    uint16_t interrupt_psm;
} swbt_btstack_device_connect_request_t;

typedef struct {
    int (*platform_start)(void *context);
    void (*platform_stop)(void *context);
    int (*hid_register)(void *context, uint8_t *service_buffer, size_t service_buffer_size,
                        const swbt_btstack_hid_registration_config_t *config);
    void (*hid_stop)(void *context);
    int (*connect)(void *context, const swbt_btstack_device_connect_request_t *request,
                   uint16_t *out_hid_cid);
    int (*send)(void *context, uint16_t hid_cid, const uint8_t *message, size_t message_size);
} swbt_btstack_device_port_t;

typedef struct {
    uint8_t *service_buffer;
    size_t service_buffer_size;
    const swbt_btstack_hid_registration_config_t *registration;
} swbt_btstack_device_open_options_t;

typedef struct {
    const swbt_btstack_device_port_t *port;
    void *port_context;
    bool initialized;
    bool platform_started;
    bool hid_registered;
} swbt_btstack_device_t;

swbt_btstack_device_result_t swbt_btstack_device_init(swbt_btstack_device_t *device,
                                                      const swbt_btstack_device_port_t *port,
                                                      void *port_context);

swbt_btstack_device_result_t swbt_btstack_device_open(swbt_btstack_device_t *device,
                                                      swbt_btstack_device_open_options_t options);

swbt_btstack_device_result_t
swbt_btstack_device_connect(swbt_btstack_device_t *device,
                            const swbt_btstack_device_connect_request_t *request,
                            uint16_t *out_hid_cid);

swbt_btstack_device_result_t swbt_btstack_device_send(swbt_btstack_device_t *device,
                                                      uint16_t hid_cid, const uint8_t *message,
                                                      size_t message_size);

swbt_btstack_device_result_t swbt_btstack_device_recv(swbt_btstack_device_t *device,
                                                      uint8_t packet_type, const uint8_t *packet,
                                                      size_t packet_size,
                                                      swbt_btstack_hid_event_t *out_event);

void swbt_btstack_device_close(swbt_btstack_device_t *device);

bool swbt_btstack_device_is_open(const swbt_btstack_device_t *device);

#endif
