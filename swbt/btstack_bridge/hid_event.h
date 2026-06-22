#ifndef SWBT_BTSTACK_BRIDGE_HID_EVENT_H
#define SWBT_BTSTACK_BRIDGE_HID_EVENT_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    SWBT_BTSTACK_HID_EVENT_OK = 0,
    SWBT_BTSTACK_HID_EVENT_IGNORED = 1,
    SWBT_BTSTACK_HID_EVENT_ERROR_INVALID_ARGUMENT = -1,
} swbt_btstack_hid_event_result_t;

typedef enum {
    SWBT_BTSTACK_HID_EVENT_NONE = 0,
    SWBT_BTSTACK_HID_EVENT_CONNECTION_OPENED,
    SWBT_BTSTACK_HID_EVENT_CONNECTION_CLOSED,
    SWBT_BTSTACK_HID_EVENT_CAN_SEND_NOW,
    SWBT_BTSTACK_HID_EVENT_USER_CONFIRMATION_REQUEST,
} swbt_btstack_hid_event_type_t;

typedef struct {
    swbt_btstack_hid_event_type_t type;
    uint16_t hid_cid;
    uint8_t status;
    uint8_t address[6];
} swbt_btstack_hid_event_t;

swbt_btstack_hid_event_result_t swbt_btstack_hid_event_decode(uint8_t packet_type,
                                                              const uint8_t *packet,
                                                              size_t packet_size,
                                                              swbt_btstack_hid_event_t *out_event);

#endif
