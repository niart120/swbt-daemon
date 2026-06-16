#include <stdint.h>
#include <string.h>

#include "core/swbt_version.h"
#include "switch/switch_controller_state.h"

int main(void) {
    swbt_state_t state = swbt_state_neutral();

    if (strcmp(swbt_get_version_string(), "0.1.0-dev") != 0) {
        return 1;
    }

    if (state.buttons != 0U) {
        return 2;
    }

    if (state.lx != 2048U || state.ly != 2048U || state.rx != 2048U || state.ry != 2048U) {
        return 3;
    }

    return 0;
}
