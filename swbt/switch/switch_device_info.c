#include "switch/switch_device_info.h"

#include <stddef.h>

swbt_switch_device_info_t swbt_switch_device_info_swbt_pro(void) {
    return (swbt_switch_device_info_t){
        .firmware_version = {0x04u, 0x00u},
        .controller_type = SWBT_SWITCH_DEVICE_INFO_CONTROLLER_TYPE_PRO_CONTROLLER,
        .bluetooth_address = {0},
        .tail_unknown = SWBT_SWITCH_DEVICE_INFO_UNKNOWN_ALWAYS_01,
        .color_source = SWBT_SWITCH_DEVICE_INFO_COLORS_FROM_SPI,
    };
}

swbt_switch_device_info_t swbt_switch_device_info_default(void) {
    return swbt_switch_device_info_swbt_pro();
}

swbt_switch_device_info_t swbt_switch_device_info_mizuyoukanao_pro(void) {
    swbt_switch_device_info_t device_info = swbt_switch_device_info_default();
    device_info.firmware_version[0] = 0x03u;
    device_info.firmware_version[1] = 0x48u;
    device_info.controller_type = SWBT_SWITCH_DEVICE_INFO_CONTROLLER_TYPE_PRO_CONTROLLER;
    device_info.tail_unknown = SWBT_SWITCH_DEVICE_INFO_MIZUYOUKANAO_PRO_TAIL_UNKNOWN;
    device_info.color_source = SWBT_SWITCH_DEVICE_INFO_MIZUYOUKANAO_PRO_COLOR_SOURCE;
    return device_info;
}

void swbt_switch_device_info_write_reply_data(
    const swbt_switch_device_info_t *device_info,
    uint8_t out_data[SWBT_SWITCH_DEVICE_INFO_REPLY_DATA_SIZE]) {
    if (device_info == NULL || out_data == NULL) {
        return;
    }

    out_data[0] = device_info->firmware_version[0];
    out_data[1] = device_info->firmware_version[1];
    out_data[2] = device_info->controller_type;
    out_data[3] = SWBT_SWITCH_DEVICE_INFO_UNKNOWN_ALWAYS_02;
    for (size_t index = 0; index < 6u; ++index) {
        out_data[4u + index] = device_info->bluetooth_address[index];
    }
    out_data[10] = device_info->tail_unknown;
    out_data[11] = device_info->color_source;
}
