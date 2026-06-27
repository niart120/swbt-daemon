#ifndef SWBT_H
#define SWBT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct swbt swbt_t;

typedef enum {
    SWBT_OK = 0,
    SWBT_ERROR_INVALID_ARGUMENT = -1,
    SWBT_ERROR_OWNER_BUSY = -2,
    SWBT_ERROR_NOT_OWNER = -3,
    SWBT_ERROR_NO_MEMORY = -4,
} swbt_result_t;

typedef enum {
    SWBT_CONTROLLER_BUTTON_Y = 1u << 0,
    SWBT_CONTROLLER_BUTTON_X = 1u << 1,
    SWBT_CONTROLLER_BUTTON_B = 1u << 2,
    SWBT_CONTROLLER_BUTTON_A = 1u << 3,
    SWBT_CONTROLLER_BUTTON_SR_R = 1u << 4,
    SWBT_CONTROLLER_BUTTON_SL_R = 1u << 5,
    SWBT_CONTROLLER_BUTTON_R = 1u << 6,
    SWBT_CONTROLLER_BUTTON_ZR = 1u << 7,
    SWBT_CONTROLLER_BUTTON_MINUS = 1u << 8,
    SWBT_CONTROLLER_BUTTON_PLUS = 1u << 9,
    SWBT_CONTROLLER_BUTTON_R_STICK = 1u << 10,
    SWBT_CONTROLLER_BUTTON_L_STICK = 1u << 11,
    SWBT_CONTROLLER_BUTTON_HOME = 1u << 12,
    SWBT_CONTROLLER_BUTTON_CAPTURE = 1u << 13,
    SWBT_CONTROLLER_BUTTON_DOWN = 1u << 16,
    SWBT_CONTROLLER_BUTTON_UP = 1u << 17,
    SWBT_CONTROLLER_BUTTON_RIGHT = 1u << 18,
    SWBT_CONTROLLER_BUTTON_LEFT = 1u << 19,
    SWBT_CONTROLLER_BUTTON_SR_L = 1u << 20,
    SWBT_CONTROLLER_BUTTON_SL_L = 1u << 21,
    SWBT_CONTROLLER_BUTTON_L = 1u << 22,
    SWBT_CONTROLLER_BUTTON_ZL = 1u << 23,
} swbt_controller_button_t;

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
} swbt_controller_state_t;

typedef struct {
    uint32_t size;
} swbt_open_options_t;

typedef struct {
    swbt_controller_state_t state;
    bool runtime_available;
    bool runtime_running;
} swbt_status_t;

const char *swbt_version_string(void);

swbt_result_t swbt_open(const swbt_open_options_t *options, swbt_t **out_swbt);
void swbt_close(swbt_t *swbt);
swbt_result_t swbt_submit_state(swbt_t *swbt, const swbt_controller_state_t *state);
swbt_result_t swbt_submit_neutral(swbt_t *swbt);
swbt_result_t swbt_get_status(swbt_t *swbt, swbt_status_t *out_status);

#ifdef __cplusplus
}
#endif

#endif
