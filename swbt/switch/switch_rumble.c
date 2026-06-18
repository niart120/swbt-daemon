#include "switch/switch_rumble.h"

const uint8_t SWBT_SWITCH_RUMBLE_NEUTRAL_PAYLOAD[SWBT_SWITCH_RUMBLE_DATA_SIZE] = {
    0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40,
};

static void swbt_switch_rumble_copy(uint8_t *out, const uint8_t *payload) {
    for (size_t index = 0; index < SWBT_SWITCH_RUMBLE_DATA_SIZE; ++index) {
        out[index] = payload[index];
    }
}

swbt_switch_rumble_result_t swbt_switch_rumble_init(swbt_switch_rumble_state_t *state) {
    if (state == NULL) {
        return SWBT_SWITCH_RUMBLE_ERROR_INVALID_ARGUMENT;
    }

    swbt_switch_rumble_copy(state->raw, SWBT_SWITCH_RUMBLE_NEUTRAL_PAYLOAD);
    state->updated_at_ms = 0;
    state->updated = false;
    return SWBT_SWITCH_RUMBLE_OK;
}

swbt_switch_rumble_result_t swbt_switch_rumble_update(swbt_switch_rumble_state_t *state,
                                                      const uint8_t *payload,
                                                      uint64_t updated_at_ms) {
    if (state == NULL || payload == NULL) {
        return SWBT_SWITCH_RUMBLE_ERROR_INVALID_ARGUMENT;
    }

    swbt_switch_rumble_copy(state->raw, payload);
    state->updated_at_ms = updated_at_ms;
    state->updated = true;
    return SWBT_SWITCH_RUMBLE_OK;
}

bool swbt_switch_rumble_payload_is_neutral(const uint8_t *payload) {
    if (payload == NULL) {
        return false;
    }

    for (size_t index = 0; index < SWBT_SWITCH_RUMBLE_DATA_SIZE; ++index) {
        if (payload[index] != SWBT_SWITCH_RUMBLE_NEUTRAL_PAYLOAD[index]) {
            return false;
        }
    }
    return true;
}

bool swbt_switch_rumble_state_is_neutral(const swbt_switch_rumble_state_t *state) {
    if (state == NULL) {
        return false;
    }

    return swbt_switch_rumble_payload_is_neutral(state->raw);
}
