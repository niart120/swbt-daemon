#include "btstack_bridge/classic_discovery_btstack_adapter.h"

#include "btstack_config.h"
#include "gap.h"

static void swbt_btstack_bridge_discoverable_control(void *context, uint8_t enable) {
    (void)context;
    gap_discoverable_control(enable);
}

static void swbt_btstack_bridge_set_class_of_device(void *context, uint32_t class_of_device) {
    (void)context;
    gap_set_class_of_device(class_of_device);
}

static void swbt_btstack_bridge_set_local_name(void *context, const char *local_name) {
    (void)context;
    gap_set_local_name(local_name);
}

static void swbt_btstack_bridge_set_default_link_policy_settings(void *context, uint16_t settings) {
    (void)context;
    gap_set_default_link_policy_settings(settings);
}

static void swbt_btstack_bridge_set_allow_role_switch(void *context, bool allow_role_switch) {
    (void)context;
    gap_set_allow_role_switch(allow_role_switch);
}

const swbt_btstack_classic_discovery_backend_t *
swbt_btstack_classic_discovery_backend_btstack(void) {
    static const swbt_btstack_classic_discovery_backend_t backend = {
        .discoverable_control = swbt_btstack_bridge_discoverable_control,
        .set_class_of_device = swbt_btstack_bridge_set_class_of_device,
        .set_local_name = swbt_btstack_bridge_set_local_name,
        .set_default_link_policy_settings = swbt_btstack_bridge_set_default_link_policy_settings,
        .set_allow_role_switch = swbt_btstack_bridge_set_allow_role_switch,
    };
    return &backend;
}
