#include <stddef.h>

#include "daemon/config.h"
#include "daemon/runtime.h"

int main(void) {
    const swbt_daemon_config_t config = swbt_daemon_config_default();
    return swbt_daemon_main_with_backend(&config, swbt_daemon_runtime_noop_backend(), NULL);
}
