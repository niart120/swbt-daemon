#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "switch/switch_player_lights.h"

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_false(bool value) {
    return value ? 1 : 0;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_size(size_t actual, size_t expected) {
    return actual == expected ? 0 : 1;
}

static int test_default_state_is_explicit_off(void) {
    swbt_switch_player_lights_state_t state;

    int failed = 0;
    failed += expect_eq_int(swbt_switch_player_lights_init(&state), SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_false(state.updated);
    failed += expect_eq_u8(state.raw, 0x00u);
    failed += expect_eq_u8(state.on_mask, 0x00u);
    failed += expect_eq_u8(state.flash_mask, 0x00u);
    failed += expect_eq_u8(state.effective_flash_mask, 0x00u);
    return failed;
}

static int test_set_request_updates_policy_state(void) {
    const uint8_t payload[] = {0x53u};
    swbt_switch_player_lights_state_t state;

    int failed = 0;
    failed += expect_eq_int(swbt_switch_player_lights_init(&state), SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_eq_int(swbt_switch_player_lights_apply_set(&state, payload, sizeof(payload)),
                            SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_true(state.updated);
    failed += expect_eq_u8(state.raw, 0x53u);
    failed += expect_eq_u8(state.on_mask, 0x03u);
    failed += expect_eq_u8(state.flash_mask, 0x05u);
    failed += expect_eq_u8(state.effective_flash_mask, 0x04u);
    return failed;
}

static int test_get_reply_returns_current_raw_policy_byte(void) {
    const uint8_t payload[] = {0x86u};
    uint8_t reply_data[1];
    size_t written = 0;
    swbt_switch_player_lights_state_t state;

    int failed = 0;
    failed += expect_eq_int(swbt_switch_player_lights_init(&state), SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_eq_int(swbt_switch_player_lights_apply_set(&state, payload, sizeof(payload)),
                            SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_eq_int(
        swbt_switch_player_lights_get_reply_data(&state, reply_data, sizeof(reply_data), &written),
        SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_eq_size(written, 1u);
    failed += expect_eq_u8(reply_data[0], 0x86u);
    return failed;
}

static int test_invalid_payload_leaves_state_unchanged(void) {
    const uint8_t payload[] = {0x21u};
    swbt_switch_player_lights_state_t state;

    int failed = 0;
    failed += expect_eq_int(swbt_switch_player_lights_init(&state), SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_eq_int(swbt_switch_player_lights_apply_set(&state, payload, sizeof(payload)),
                            SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_eq_int(swbt_switch_player_lights_apply_set(&state, NULL, 1u),
                            SWBT_SWITCH_PLAYER_LIGHTS_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_switch_player_lights_apply_set(&state, payload, 0u),
                            SWBT_SWITCH_PLAYER_LIGHTS_ERROR_INVALID_ARGUMENT);
    failed += expect_true(state.updated);
    failed += expect_eq_u8(state.raw, 0x21u);
    failed += expect_eq_u8(state.on_mask, 0x01u);
    failed += expect_eq_u8(state.flash_mask, 0x02u);
    failed += expect_eq_u8(state.effective_flash_mask, 0x02u);
    return failed;
}

static int test_rejects_invalid_reply_buffer(void) {
    uint8_t reply_data[1];
    size_t written = 99u;
    swbt_switch_player_lights_state_t state;

    int failed = 0;
    failed += expect_eq_int(swbt_switch_player_lights_init(&state), SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_eq_int(
        swbt_switch_player_lights_get_reply_data(NULL, reply_data, sizeof(reply_data), &written),
        SWBT_SWITCH_PLAYER_LIGHTS_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(
        swbt_switch_player_lights_get_reply_data(&state, NULL, sizeof(reply_data), &written),
        SWBT_SWITCH_PLAYER_LIGHTS_ERROR_INVALID_ARGUMENT);
    failed +=
        expect_eq_int(swbt_switch_player_lights_get_reply_data(&state, reply_data, 0u, &written),
                      SWBT_SWITCH_PLAYER_LIGHTS_ERROR_BUFFER_TOO_SMALL);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += test_default_state_is_explicit_off();
    failed += test_set_request_updates_policy_state();
    failed += test_get_reply_returns_current_raw_policy_byte();
    failed += test_invalid_payload_leaves_state_unchanged();
    failed += test_rejects_invalid_reply_buffer();
    return failed == 0 ? 0 : 1;
}
