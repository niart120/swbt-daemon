#include "switch/switch_player_lights.h"

static void swbt_switch_player_lights_apply_raw(swbt_switch_player_lights_state_t *state,
                                                uint8_t raw) {
    const uint8_t on_mask = raw & SWBT_SWITCH_PLAYER_LIGHTS_MASK;
    const uint8_t flash_mask = (raw >> 4u) & SWBT_SWITCH_PLAYER_LIGHTS_MASK;

    state->raw = raw;
    state->on_mask = on_mask;
    state->flash_mask = flash_mask;
    state->effective_flash_mask = flash_mask & (uint8_t)(~on_mask) & SWBT_SWITCH_PLAYER_LIGHTS_MASK;
    state->updated = true;
}

swbt_switch_player_lights_result_t
swbt_switch_player_lights_init(swbt_switch_player_lights_state_t *state) {
    if (state == NULL) {
        return SWBT_SWITCH_PLAYER_LIGHTS_ERROR_INVALID_ARGUMENT;
    }

    state->raw = 0;
    state->on_mask = 0;
    state->flash_mask = 0;
    state->effective_flash_mask = 0;
    state->updated = false;
    return SWBT_SWITCH_PLAYER_LIGHTS_OK;
}

swbt_switch_player_lights_result_t
swbt_switch_player_lights_apply_set(swbt_switch_player_lights_state_t *state,
                                    const uint8_t *payload, size_t payload_len) {
    if (state == NULL || payload == NULL ||
        payload_len < SWBT_SWITCH_PLAYER_LIGHTS_REPLY_DATA_SIZE) {
        return SWBT_SWITCH_PLAYER_LIGHTS_ERROR_INVALID_ARGUMENT;
    }

    swbt_switch_player_lights_apply_raw(state, payload[0]);
    return SWBT_SWITCH_PLAYER_LIGHTS_OK;
}

swbt_switch_player_lights_result_t
swbt_switch_player_lights_get_reply_data(const swbt_switch_player_lights_state_t *state,
                                         uint8_t *out_data, size_t out_data_size,
                                         size_t *out_written) {
    if (out_written != NULL) {
        *out_written = 0;
    }
    if (state == NULL || out_data == NULL || out_written == NULL) {
        return SWBT_SWITCH_PLAYER_LIGHTS_ERROR_INVALID_ARGUMENT;
    }
    if (out_data_size < SWBT_SWITCH_PLAYER_LIGHTS_REPLY_DATA_SIZE) {
        return SWBT_SWITCH_PLAYER_LIGHTS_ERROR_BUFFER_TOO_SMALL;
    }

    out_data[0] = state->raw;
    *out_written = SWBT_SWITCH_PLAYER_LIGHTS_REPLY_DATA_SIZE;
    return SWBT_SWITCH_PLAYER_LIGHTS_OK;
}
