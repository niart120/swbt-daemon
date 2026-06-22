#include "btstack_bridge/hid_event.h"

#include <stddef.h>
#include <stdint.h>

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_event_type(swbt_btstack_hid_event_type_t actual,
                                swbt_btstack_hid_event_type_t expected) {
    return actual == expected ? 0 : 1;
}

static int connection_opened_packet_decodes_to_typed_event(void) {
    const uint8_t packet[] = {0xefu, 13u, 0x02u, 0x42u, 0x00u, 0x00u, 0u, 0u,
                              0u,    0u,  0u,    0u,    0u,    0u,    1u};
    swbt_btstack_hid_event_t event;

    const swbt_btstack_hid_event_result_t result =
        swbt_btstack_hid_event_decode(0x04u, packet, sizeof(packet), &event);

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_HID_EVENT_OK);
    failed += expect_eq_event_type(event.type, SWBT_BTSTACK_HID_EVENT_CONNECTION_OPENED);
    failed += expect_eq_u16(event.hid_cid, 0x0042u);
    failed += expect_eq_u8(event.status, 0u);
    return failed;
}

static int can_send_packet_decodes_to_typed_event(void) {
    const uint8_t packet[] = {0xefu, 3u, 0x04u, 0x42u, 0x00u};
    swbt_btstack_hid_event_t event;

    const swbt_btstack_hid_event_result_t result =
        swbt_btstack_hid_event_decode(0x04u, packet, sizeof(packet), &event);

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_HID_EVENT_OK);
    failed += expect_eq_event_type(event.type, SWBT_BTSTACK_HID_EVENT_CAN_SEND_NOW);
    failed += expect_eq_u16(event.hid_cid, 0x0042u);
    return failed;
}

static int connection_closed_packet_decodes_to_typed_event(void) {
    const uint8_t packet[] = {0xefu, 3u, 0x03u, 0x42u, 0x00u};
    swbt_btstack_hid_event_t event;

    const swbt_btstack_hid_event_result_t result =
        swbt_btstack_hid_event_decode(0x04u, packet, sizeof(packet), &event);

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_HID_EVENT_OK);
    failed += expect_eq_event_type(event.type, SWBT_BTSTACK_HID_EVENT_CONNECTION_CLOSED);
    failed += expect_eq_u16(event.hid_cid, 0x0042u);
    return failed;
}

static int user_confirmation_packet_decodes_to_typed_event(void) {
    const uint8_t packet[] = {0x33u, 0x0au, 0x21u, 0xb5u, 0xf7u, 0x05u,
                              0x48u, 0xc8u, 0xc4u, 0xdcu, 0x09u, 0x00u};
    const uint8_t expected_address[] = {0xc8u, 0x48u, 0x05u, 0xf7u, 0xb5u, 0x21u};
    swbt_btstack_hid_event_t event;

    const swbt_btstack_hid_event_result_t result =
        swbt_btstack_hid_event_decode(0x04u, packet, sizeof(packet), &event);

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_HID_EVENT_OK);
    failed += expect_eq_event_type(event.type, SWBT_BTSTACK_HID_EVENT_USER_CONFIRMATION_REQUEST);
    for (size_t index = 0; index < sizeof(expected_address); ++index) {
        failed += expect_eq_u8(event.address[index], expected_address[index]);
    }
    return failed;
}

static int non_hci_or_short_packets_are_ignored(void) {
    const uint8_t packet[] = {0xefu, 3u, 0x04u, 0x42u};
    const uint8_t short_opened_packet[] = {0xefu, 13u, 0x02u, 0x42u, 0x00u, 0x00u};
    swbt_btstack_hid_event_t event = {
        .type = SWBT_BTSTACK_HID_EVENT_CONNECTION_OPENED,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_hid_event_decode(0x01u, packet, sizeof(packet), &event),
                            SWBT_BTSTACK_HID_EVENT_IGNORED);
    failed += expect_eq_event_type(event.type, SWBT_BTSTACK_HID_EVENT_NONE);
    failed += expect_eq_int(swbt_btstack_hid_event_decode(0x04u, packet, sizeof(packet), &event),
                            SWBT_BTSTACK_HID_EVENT_IGNORED);
    failed += expect_eq_event_type(event.type, SWBT_BTSTACK_HID_EVENT_NONE);
    failed += expect_eq_int(swbt_btstack_hid_event_decode(0x04u, short_opened_packet,
                                                          sizeof(short_opened_packet), &event),
                            SWBT_BTSTACK_HID_EVENT_IGNORED);
    failed += expect_eq_event_type(event.type, SWBT_BTSTACK_HID_EVENT_NONE);
    failed += expect_eq_int(swbt_btstack_hid_event_decode(0x04u, NULL, 0u, &event),
                            SWBT_BTSTACK_HID_EVENT_IGNORED);
    return failed;
}

static int invalid_out_event_is_rejected(void) {
    const uint8_t packet[] = {0xefu, 3u, 0x04u, 0x42u, 0x00u};
    return expect_eq_int(swbt_btstack_hid_event_decode(0x04u, packet, sizeof(packet), NULL),
                         SWBT_BTSTACK_HID_EVENT_ERROR_INVALID_ARGUMENT);
}

int main(void) {
    int failed = 0;
    failed += connection_opened_packet_decodes_to_typed_event();
    failed += can_send_packet_decodes_to_typed_event();
    failed += connection_closed_packet_decodes_to_typed_event();
    failed += user_confirmation_packet_decodes_to_typed_event();
    failed += non_hci_or_short_packets_are_ignored();
    failed += invalid_out_event_is_rejected();
    return failed == 0 ? 0 : 1;
}
