#ifndef SWBT_DAEMON_SWITCH_ADDRESS_H
#define SWBT_DAEMON_SWITCH_ADDRESS_H

#include <stdbool.h>

#define SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE 18u

bool swbt_daemon_switch_address_normalize(char dest[SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE],
                                          const char *address);

#endif
