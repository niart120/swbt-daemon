#ifndef SWBT_SWITCH_HID_DESCRIPTOR_H
#define SWBT_SWITCH_HID_DESCRIPTOR_H

#include <stddef.h>
#include <stdint.h>

#define SWBT_SWITCH_HID_DESCRIPTOR_SIZE 203u

const uint8_t *swbt_switch_hid_descriptor_data(void);

size_t swbt_switch_hid_descriptor_size(void);

#endif
