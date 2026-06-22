#include "btstack_bridge/hid_port.h"

#include <stdbool.h>

#include "classic/hid_device.h"

static bool swbt_btstack_hid_port_is_valid(const swbt_btstack_hid_port_t *port) {
    return port != NULL && port->request_can_send_now_event != NULL &&
           port->send_interrupt_message != NULL;
}

static void swbt_btstack_hid_port_btstack_request_can_send(uint16_t hid_cid) {
    hid_device_request_can_send_now_event(hid_cid);
}

static int swbt_btstack_hid_port_btstack_send_interrupt(uint16_t hid_cid, const uint8_t *message,
                                                        uint16_t message_len) {
    hid_device_send_interrupt_message(hid_cid, message, message_len);
    return 0;
}

const swbt_btstack_hid_port_t *swbt_btstack_hid_port_btstack(void) {
    static const swbt_btstack_hid_port_t port = {
        .request_can_send_now_event = swbt_btstack_hid_port_btstack_request_can_send,
        .send_interrupt_message = swbt_btstack_hid_port_btstack_send_interrupt,
    };
    return &port;
}

swbt_btstack_hid_port_result_t
swbt_btstack_hid_port_request_can_send_now(const swbt_btstack_hid_port_t *port, uint16_t hid_cid) {
    if (!swbt_btstack_hid_port_is_valid(port)) {
        return SWBT_BTSTACK_HID_PORT_ERROR_INVALID_ARGUMENT;
    }

    port->request_can_send_now_event(hid_cid);
    return SWBT_BTSTACK_HID_PORT_OK;
}

swbt_btstack_hid_port_result_t
swbt_btstack_hid_port_send_report(const swbt_btstack_hid_port_t *port, uint16_t hid_cid,
                                  const uint8_t *report, size_t report_size) {
    if (!swbt_btstack_hid_port_is_valid(port) || report == NULL || report_size == 0u ||
        report_size > UINT16_MAX) {
        return SWBT_BTSTACK_HID_PORT_ERROR_INVALID_ARGUMENT;
    }

    return port->send_interrupt_message(hid_cid, report, (uint16_t)report_size) == 0
               ? SWBT_BTSTACK_HID_PORT_OK
               : SWBT_BTSTACK_HID_PORT_ERROR_SEND_FAILED;
}
