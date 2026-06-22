#include "switch/switch_controller_state.h"

swbt_state_t swbt_state_neutral(void) {
    swbt_state_t state = {
        .buttons = 0,
        .lx = 2048,
        .ly = 2048,
        .rx = 2048,
        .ry = 2048,
        .accel_x = 0,
        .accel_y = 0,
        .accel_z = 0,
        .gyro_x = 0,
        .gyro_y = 0,
        .gyro_z = 0,
    };
    return state;
}
