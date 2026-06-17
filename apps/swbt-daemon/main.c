#include <stdio.h>

#include "core/swbt_version.h"
#include "switch/switch_controller_state.h"

int main(void) {
    swbt_state_t state = swbt_state_neutral();

    printf("swbt-daemon %s\n", swbt_get_version_string());
    printf("neutral state: buttons=%u lx=%u ly=%u rx=%u ry=%u\n", (unsigned)state.buttons,
           (unsigned)state.lx, (unsigned)state.ly, (unsigned)state.rx, (unsigned)state.ry);

    return 0;
}
