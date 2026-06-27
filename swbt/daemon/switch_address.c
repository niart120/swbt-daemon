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
