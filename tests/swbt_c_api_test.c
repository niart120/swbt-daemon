#include "swbt.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_false(bool value) {
    return !value ? 0 : 1;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_i16(int16_t actual, int16_t expected) {
    return actual == expected ? 0 : 1;
}

static int open_close_rejects_invalid_arguments(void) {
    swbt_t *handle = NULL;
    int failed = 0;

    failed += expect_eq_int(swbt_open(NULL, NULL), SWBT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_open(NULL, &handle), SWBT_OK);
    failed += expect_true(handle != NULL);

    swbt_close(NULL);
    swbt_close(handle);
    return failed;
}

static int submit_state_and_status_use_public_abi_only(void) {
    swbt_t *handle = NULL;
    swbt_status_t status = {0};
    const swbt_controller_state_t state = {
        .buttons = SWBT_CONTROLLER_BUTTON_A | SWBT_CONTROLLER_BUTTON_ZR,
        .lx = 1111u,
        .ly = 2222u,
        .rx = 3333u,
        .ry = 4444u,
        .accel_x = -1,
        .accel_y = -2,
        .accel_z = -3,
        .gyro_x = 4,
        .gyro_y = 5,
        .gyro_z = 6,
    };
    int failed = 0;

    failed += expect_eq_int(swbt_open(NULL, &handle), SWBT_OK);
    failed += expect_true(handle != NULL);
    failed += expect_eq_int(swbt_get_status(handle, &status), SWBT_OK);
    failed += expect_eq_u32(status.state.buttons, 0u);
    failed += expect_eq_u16(status.state.lx, 2048u);
    failed += expect_false(status.runtime_available);
    failed += expect_false(status.runtime_running);

    failed += expect_eq_int(swbt_submit_state(NULL, &state), SWBT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_submit_state(handle, NULL), SWBT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_submit_state(handle, &state), SWBT_OK);
    failed += expect_eq_int(swbt_get_status(handle, &status), SWBT_OK);
    failed +=
        expect_eq_u32(status.state.buttons, SWBT_CONTROLLER_BUTTON_A | SWBT_CONTROLLER_BUTTON_ZR);
    failed += expect_eq_u16(status.state.lx, 1111u);
    failed += expect_eq_u16(status.state.ly, 2222u);
    failed += expect_eq_u16(status.state.rx, 3333u);
    failed += expect_eq_u16(status.state.ry, 4444u);
    failed += expect_eq_i16(status.state.accel_x, -1);
    failed += expect_eq_i16(status.state.accel_y, -2);
    failed += expect_eq_i16(status.state.accel_z, -3);
    failed += expect_eq_i16(status.state.gyro_x, 4);
    failed += expect_eq_i16(status.state.gyro_y, 5);
    failed += expect_eq_i16(status.state.gyro_z, 6);

    failed += expect_eq_int(swbt_submit_neutral(handle), SWBT_OK);
    failed += expect_eq_int(swbt_get_status(handle, &status), SWBT_OK);
    failed += expect_eq_u32(status.state.buttons, 0u);
    failed += expect_eq_u16(status.state.lx, 2048u);
    failed += expect_eq_u16(status.state.ly, 2048u);
    failed += expect_eq_u16(status.state.rx, 2048u);
    failed += expect_eq_u16(status.state.ry, 2048u);

    swbt_close(handle);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += open_close_rejects_invalid_arguments();
    failed += submit_state_and_status_use_public_abi_only();
    return failed == 0 ? 0 : 1;
}
