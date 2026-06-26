#include "btstack_bridge/hid_event.h"

#include <stdbool.h>

#include "bluetooth.h"
#include "btstack_defines.h"

static void swbt_btstack_hid_event_clear(swbt_btstack_hid_event_t *event) {
    *event = (swbt_btstack_hid_event_t){0};
}

static bool swbt_btstack_hid_event_has_prefix(const uint8_t *packet, size_t packet_size) {
    return packet != NULL && packet_size >= 1u;
}

static bool swbt_btstack_hid_event_is_hid_meta(const uint8_t *packet, size_t packet_size) {
    return packet_size >= 3u && packet[0] == HCI_EVENT_HID_META;
}

static uint16_t swbt_btstack_hid_event_read_u16_le(const uint8_t *data) {
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static void swbt_btstack_hid_event_copy_reversed_address(const uint8_t *address_source,
                                                         uint8_t address[6]) {
    address[0] = address_source[5];
    address[1] = address_source[4];
    address[2] = address_source[3];
    address[3] = address_source[2];
    address[4] = address_source[1];
    address[5] = address_source[0];
}

swbt_btstack_hid_event_result_t swbt_btstack_hid_event_decode(uint8_t packet_type,
                                                              const uint8_t *packet,
                                                              size_t packet_size,
                                                              swbt_btstack_hid_event_t *out_event) {
    if (out_event == NULL) {
        return SWBT_BTSTACK_HID_EVENT_ERROR_INVALID_ARGUMENT;
    }
    swbt_btstack_hid_event_clear(out_event);

    if (packet_type != HCI_EVENT_PACKET ||
        !swbt_btstack_hid_event_has_prefix(packet, packet_size)) {
        return SWBT_BTSTACK_HID_EVENT_IGNORED;
    }

    if (packet[0] == HCI_EVENT_USER_CONFIRMATION_REQUEST) {
        if (packet_size < 8u) {
            return SWBT_BTSTACK_HID_EVENT_IGNORED;
        }
        out_event->type = SWBT_BTSTACK_HID_EVENT_USER_CONFIRMATION_REQUEST;
        swbt_btstack_hid_event_copy_reversed_address(&packet[2], out_event->address);
        return SWBT_BTSTACK_HID_EVENT_OK;
    }

    if (!swbt_btstack_hid_event_is_hid_meta(packet, packet_size)) {
        return SWBT_BTSTACK_HID_EVENT_IGNORED;
    }

    switch (packet[2]) {
    case HID_SUBEVENT_CONNECTION_OPENED:
        if (packet_size < 15u) {
            return SWBT_BTSTACK_HID_EVENT_IGNORED;
        }
        out_event->type = SWBT_BTSTACK_HID_EVENT_CONNECTION_OPENED;
        out_event->hid_cid = swbt_btstack_hid_event_read_u16_le(&packet[3]);
        out_event->status = packet[5];
        swbt_btstack_hid_event_copy_reversed_address(&packet[6], out_event->address);
        return SWBT_BTSTACK_HID_EVENT_OK;
    case HID_SUBEVENT_CONNECTION_CLOSED:
        if (packet_size < 5u) {
            return SWBT_BTSTACK_HID_EVENT_IGNORED;
        }
        out_event->type = SWBT_BTSTACK_HID_EVENT_CONNECTION_CLOSED;
        out_event->hid_cid = swbt_btstack_hid_event_read_u16_le(&packet[3]);
        return SWBT_BTSTACK_HID_EVENT_OK;
    case HID_SUBEVENT_CAN_SEND_NOW:
        if (packet_size < 5u) {
            return SWBT_BTSTACK_HID_EVENT_IGNORED;
        }
        out_event->type = SWBT_BTSTACK_HID_EVENT_CAN_SEND_NOW;
        out_event->hid_cid = swbt_btstack_hid_event_read_u16_le(&packet[3]);
        return SWBT_BTSTACK_HID_EVENT_OK;
    default:
        return SWBT_BTSTACK_HID_EVENT_IGNORED;
    }
}
