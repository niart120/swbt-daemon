#ifndef SWBT_SWITCH_CONTROLLER_STATE_H
#define SWBT_SWITCH_CONTROLLER_STATE_H

#include <stdint.h>

typedef struct {
    uint32_t buttons;

    uint16_t lx;
    uint16_t ly;
    uint16_t rx;
    uint16_t ry;

    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;

    uint64_t client_seq;
} swbt_state_t;

swbt_state_t swbt_state_neutral(void);

#endif
