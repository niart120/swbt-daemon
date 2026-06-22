#ifndef SWBT_BTSTACK_BRIDGE_HID_PORT_H
#define SWBT_BTSTACK_BRIDGE_HID_PORT_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    SWBT_BTSTACK_HID_PORT_OK = 0,
    SWBT_BTSTACK_HID_PORT_ERROR_INVALID_ARGUMENT = -1,
    SWBT_BTSTACK_HID_PORT_ERROR_SEND_FAILED = -2,
} swbt_btstack_hid_port_result_t;

typedef struct {
    void (*request_can_send_now_event)(uint16_t hid_cid);
    int (*send_interrupt_message)(uint16_t hid_cid, const uint8_t *message, uint16_t message_len);
} swbt_btstack_hid_port_t;

const swbt_btstack_hid_port_t *swbt_btstack_hid_port_btstack(void);

swbt_btstack_hid_port_result_t
swbt_btstack_hid_port_request_can_send_now(const swbt_btstack_hid_port_t *port, uint16_t hid_cid);

swbt_btstack_hid_port_result_t
swbt_btstack_hid_port_send_report(const swbt_btstack_hid_port_t *port, uint16_t hid_cid,
                                  const uint8_t *report, size_t report_size);

#endif
