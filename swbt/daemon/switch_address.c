#include "daemon/switch_address.h"

#include <stddef.h>

static bool swbt_daemon_switch_address_hex_digit_to_upper(char value, char *out_value) {
    if (out_value == NULL) {
        return false;
    }
    if (value >= '0' && value <= '9') {
        *out_value = value;
        return true;
    }
    if (value >= 'a' && value <= 'f') {
        *out_value = (char)(value - 'a' + 'A');
        return true;
    }
    if (value >= 'A' && value <= 'F') {
        *out_value = value;
        return true;
    }
    return false;
}

static bool swbt_daemon_switch_address_hex_nibble(char value, uint8_t *out_nibble) {
    if (out_nibble == NULL) {
        return false;
    }
    if (value >= '0' && value <= '9') {
        *out_nibble = (uint8_t)(value - '0');
        return true;
    }
    if (value >= 'a' && value <= 'f') {
        *out_nibble = (uint8_t)(value - 'a' + 10);
        return true;
    }
    if (value >= 'A' && value <= 'F') {
        *out_nibble = (uint8_t)(value - 'A' + 10);
        return true;
    }
    return false;
}

bool swbt_daemon_switch_address_normalize(char dest[SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE],
                                          const char *address) {
    char normalized[SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE];

    if (dest == NULL || address == NULL) {
        return false;
    }
    for (size_t index = 0u; index + 1u < SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE; ++index) {
        if (address[index] == '\0') {
            return false;
        }
        if ((index + 1u) % 3u == 0u) {
            if (address[index] != ':') {
                return false;
            }
            normalized[index] = ':';
        } else if (!swbt_daemon_switch_address_hex_digit_to_upper(address[index],
                                                                  &normalized[index])) {
            return false;
        }
    }
    if (address[SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE - 1u] != '\0') {
        return false;
    }
    normalized[SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE - 1u] = '\0';
    for (size_t index = 0u; index < SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE; ++index) {
        dest[index] = normalized[index];
    }
    return true;
}

bool swbt_daemon_switch_address_parse_bytes(const char *text, uint8_t address[6]) {
    if (text == NULL || address == NULL) {
        return false;
    }
    for (size_t index = 0u; index < 6u; ++index) {
        const size_t offset = index * 3u;
        uint8_t high = 0u;
        uint8_t low = 0u;
        if (!swbt_daemon_switch_address_hex_nibble(text[offset], &high) ||
            !swbt_daemon_switch_address_hex_nibble(text[offset + 1u], &low)) {
            return false;
        }
        if (index < 5u && text[offset + 2u] != ':') {
            return false;
        }
        address[index] = (uint8_t)((uint8_t)(high << 4u) | low);
    }
    return text[SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE - 1u] == '\0';
}
