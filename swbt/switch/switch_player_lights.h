#ifndef SWBT_SWITCH_PLAYER_LIGHTS_H
#define SWBT_SWITCH_PLAYER_LIGHTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SWBT_SWITCH_PLAYER_LIGHTS_MASK 0x0Fu
#define SWBT_SWITCH_PLAYER_LIGHTS_REPLY_DATA_SIZE 1u

typedef enum {
    SWBT_SWITCH_PLAYER_LIGHTS_OK = 0,
    SWBT_SWITCH_PLAYER_LIGHTS_ERROR_INVALID_ARGUMENT = -1,
    SWBT_SWITCH_PLAYER_LIGHTS_ERROR_BUFFER_TOO_SMALL = -2,
} swbt_switch_player_lights_result_t;

typedef struct {
    uint8_t raw;
    uint8_t on_mask;
    uint8_t flash_mask;
    uint8_t effective_flash_mask;
    bool updated;
} swbt_switch_player_lights_state_t;

swbt_switch_player_lights_result_t
swbt_switch_player_lights_init(swbt_switch_player_lights_state_t *state);

swbt_switch_player_lights_result_t
swbt_switch_player_lights_apply_set(swbt_switch_player_lights_state_t *state,
                                    const uint8_t *payload, size_t payload_len);

swbt_switch_player_lights_result_t
swbt_switch_player_lights_get_reply_data(const swbt_switch_player_lights_state_t *state,
                                         uint8_t *out_data, size_t out_data_size,
                                         size_t *out_written);

#endif
