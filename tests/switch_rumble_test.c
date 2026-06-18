#include <stdbool.h>
#include <stdint.h>

#include "switch/switch_rumble.h"

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_false(bool value) {
    return value ? 1 : 0;
}

static int expect_eq_u64(uint64_t actual, uint64_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_payload(const uint8_t *actual, const uint8_t *expected) {
    for (size_t index = 0; index < SWBT_SWITCH_RUMBLE_DATA_SIZE; ++index) {
        if (actual[index] != expected[index]) {
            return 1;
        }
    }
    return 0;
}

int main(void) {
    swbt_switch_rumble_state_t state;
    const uint8_t active_payload[SWBT_SWITCH_RUMBLE_DATA_SIZE] = {
        0x04, 0x01, 0x80, 0x41, 0x08, 0x01, 0x80, 0x42,
    };

    if (swbt_switch_rumble_init(&state) != SWBT_SWITCH_RUMBLE_OK) {
        return 1;
    }
    if (expect_false(state.updated)) {
        return 2;
    }
    if (expect_true(swbt_switch_rumble_state_is_neutral(&state))) {
        return 3;
    }
    if (expect_true(swbt_switch_rumble_payload_is_neutral(SWBT_SWITCH_RUMBLE_NEUTRAL_PAYLOAD))) {
        return 4;
    }
    if (expect_false(swbt_switch_rumble_payload_is_neutral(active_payload))) {
        return 5;
    }
    if (swbt_switch_rumble_update(&state, active_payload, 1234) != SWBT_SWITCH_RUMBLE_OK) {
        return 6;
    }
    if (expect_true(state.updated)) {
        return 7;
    }
    if (expect_eq_u64(state.updated_at_ms, 1234)) {
        return 8;
    }
    if (expect_payload(state.raw, active_payload)) {
        return 9;
    }
    if (expect_false(swbt_switch_rumble_state_is_neutral(&state))) {
        return 10;
    }
    if (swbt_switch_rumble_update(&state, SWBT_SWITCH_RUMBLE_NEUTRAL_PAYLOAD, 5678) !=
        SWBT_SWITCH_RUMBLE_OK) {
        return 11;
    }
    if (expect_true(swbt_switch_rumble_state_is_neutral(&state))) {
        return 12;
    }
    if (expect_eq_u64(state.updated_at_ms, 5678)) {
        return 13;
    }
    if (swbt_switch_rumble_init(NULL) != SWBT_SWITCH_RUMBLE_ERROR_INVALID_ARGUMENT) {
        return 14;
    }
    if (swbt_switch_rumble_update(NULL, active_payload, 0) !=
        SWBT_SWITCH_RUMBLE_ERROR_INVALID_ARGUMENT) {
        return 15;
    }
    if (swbt_switch_rumble_update(&state, NULL, 0) != SWBT_SWITCH_RUMBLE_ERROR_INVALID_ARGUMENT) {
        return 16;
    }
    if (expect_false(swbt_switch_rumble_payload_is_neutral(NULL))) {
        return 17;
    }

    return 0;
}
