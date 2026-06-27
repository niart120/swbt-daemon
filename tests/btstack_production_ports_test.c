#include "btstack_bridge/production_ports.h"

static int expect_true(int actual, const char *label) {
    (void)label;
    return actual != 0 ? 0 : 1;
}

static int fake_ipc_start(void *context, const swbt_btstack_production_ipc_pump_t *pump) {
    (void)context;
    (void)pump;
    return 0;
}

static void fake_ipc_stop(void *context) {
    (void)context;
}

static swbt_btstack_production_ports_t ipc_pump_only_ports(void) {
    return (swbt_btstack_production_ports_t){
        .ipc_pump =
            {
                .start = fake_ipc_start,
                .stop = fake_ipc_stop,
            },
    };
}

static int ipc_pump_only_ports_pass_initialization_validation(void) {
    const swbt_btstack_production_ports_t ports = ipc_pump_only_ports();
    return expect_true(swbt_btstack_production_ports_has_ipc_pump(&ports), "ipc pump validation");
}

static int ipc_pump_only_ports_fail_startup_validation(void) {
    const swbt_btstack_production_ports_t ports = ipc_pump_only_ports();
    return expect_true(!swbt_btstack_production_ports_is_valid(&ports),
                       "startup validation rejects IPC-only ports");
}

int main(void) {
    int failed = 0;
    failed += ipc_pump_only_ports_pass_initialization_validation();
    failed += ipc_pump_only_ports_fail_startup_validation();
    return failed == 0 ? 0 : 1;
}
