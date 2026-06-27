#ifndef SWBT_DAEMON_SWITCH_ADDRESS_H
#define SWBT_DAEMON_SWITCH_ADDRESS_H

#include <stdbool.h>
#include <stdint.h>

#define SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE 18u

bool swbt_daemon_switch_address_normalize(char dest[SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE],
                                          const char *address);

bool swbt_daemon_switch_address_parse_bytes(const char *text, uint8_t address[6]);

#endif
