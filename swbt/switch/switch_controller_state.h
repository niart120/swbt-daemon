#ifndef SWBT_SWITCH_CONTROLLER_STATE_H
#define SWBT_SWITCH_CONTROLLER_STATE_H

#include <stdint.h>

enum {
    SWBT_BUTTON_Y = 1u << 0,
    SWBT_BUTTON_X = 1u << 1,
    SWBT_BUTTON_B = 1u << 2,
    SWBT_BUTTON_A = 1u << 3,
    SWBT_BUTTON_SR_R = 1u << 4,
    SWBT_BUTTON_SL_R = 1u << 5,
    SWBT_BUTTON_R = 1u << 6,
    SWBT_BUTTON_ZR = 1u << 7,
    SWBT_BUTTON_MINUS = 1u << 8,
    SWBT_BUTTON_PLUS = 1u << 9,
    SWBT_BUTTON_R_STICK = 1u << 10,
    SWBT_BUTTON_L_STICK = 1u << 11,
    SWBT_BUTTON_HOME = 1u << 12,
    SWBT_BUTTON_CAPTURE = 1u << 13,
    SWBT_BUTTON_DOWN = 1u << 16,
    SWBT_BUTTON_UP = 1u << 17,
    SWBT_BUTTON_RIGHT = 1u << 18,
    SWBT_BUTTON_LEFT = 1u << 19,
    SWBT_BUTTON_SR_L = 1u << 20,
    SWBT_BUTTON_SL_L = 1u << 21,
    SWBT_BUTTON_L = 1u << 22,
    SWBT_BUTTON_ZL = 1u << 23,
};

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
} swbt_state_t;

swbt_state_t swbt_state_neutral(void);

#endif
