#ifndef SWBT_SWITCH_DEVICE_INFO_H
#define SWBT_SWITCH_DEVICE_INFO_H

#include <stdint.h>

#define SWBT_SWITCH_DEVICE_INFO_REPLY_DATA_SIZE 12u
#define SWBT_SWITCH_DEVICE_INFO_CONTROLLER_TYPE_PRO_CONTROLLER 0x03u
#define SWBT_SWITCH_DEVICE_INFO_UNKNOWN_ALWAYS_02 0x02u
#define SWBT_SWITCH_DEVICE_INFO_UNKNOWN_ALWAYS_01 0x01u
#define SWBT_SWITCH_DEVICE_INFO_COLORS_FROM_SPI 0x01u
#define SWBT_SWITCH_DEVICE_INFO_MIZUYOUKANAO_PRO_TAIL_UNKNOWN 0x03u
#define SWBT_SWITCH_DEVICE_INFO_MIZUYOUKANAO_PRO_COLOR_SOURCE 0x02u

typedef struct {
    uint8_t firmware_version[2];
    uint8_t controller_type;
    uint8_t bluetooth_address[6];
    uint8_t tail_unknown;
    uint8_t color_source;
} swbt_switch_device_info_t;

swbt_switch_device_info_t swbt_switch_device_info_swbt_pro(void);

swbt_switch_device_info_t swbt_switch_device_info_default(void);

swbt_switch_device_info_t swbt_switch_device_info_mizuyoukanao_pro(void);

void swbt_switch_device_info_write_reply_data(
    const swbt_switch_device_info_t *device_info,
    uint8_t out_data[SWBT_SWITCH_DEVICE_INFO_REPLY_DATA_SIZE]);

#endif
