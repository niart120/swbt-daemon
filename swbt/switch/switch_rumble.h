#ifndef SWBT_SWITCH_RUMBLE_H
#define SWBT_SWITCH_RUMBLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SWBT_SWITCH_RUMBLE_DATA_SIZE 8u

extern const uint8_t SWBT_SWITCH_RUMBLE_NEUTRAL_PAYLOAD[SWBT_SWITCH_RUMBLE_DATA_SIZE];

typedef enum {
    SWBT_SWITCH_RUMBLE_OK = 0,
    SWBT_SWITCH_RUMBLE_ERROR_INVALID_ARGUMENT = -1,
} swbt_switch_rumble_result_t;

typedef struct {
    uint8_t raw[SWBT_SWITCH_RUMBLE_DATA_SIZE];
    uint64_t updated_at_ms;
    bool updated;
} swbt_switch_rumble_state_t;

swbt_switch_rumble_result_t swbt_switch_rumble_init(swbt_switch_rumble_state_t *state);

swbt_switch_rumble_result_t swbt_switch_rumble_update(swbt_switch_rumble_state_t *state,
                                                      const uint8_t *payload,
                                                      uint64_t updated_at_ms);

bool swbt_switch_rumble_payload_is_neutral(const uint8_t *payload);

bool swbt_switch_rumble_state_is_neutral(const swbt_switch_rumble_state_t *state);

#endif
