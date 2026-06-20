#include "btstack_bridge/classic_discovery.h"

#include <stddef.h>

static bool swbt_btstack_classic_discovery_backend_is_valid(
    const swbt_btstack_classic_discovery_backend_t *backend) {
    return backend != NULL && backend->discoverable_control != NULL &&
           backend->set_class_of_device != NULL && backend->set_local_name != NULL &&
           backend->set_default_link_policy_settings != NULL &&
           backend->set_allow_role_switch != NULL;
}

swbt_btstack_classic_discovery_result_t
swbt_btstack_classic_discovery_configure(const swbt_btstack_classic_discovery_backend_t *backend,
                                         void *backend_context,
                                         const swbt_btstack_classic_discovery_config_t *config) {
    if (!swbt_btstack_classic_discovery_backend_is_valid(backend) || config == NULL ||
        config->local_name == NULL || config->local_name[0] == '\0') {
        return SWBT_BTSTACK_CLASSIC_DISCOVERY_ERROR_INVALID_ARGUMENT;
    }

    backend->discoverable_control(backend_context, config->discoverable ? 1u : 0u);
    backend->set_class_of_device(backend_context, config->class_of_device);
    backend->set_local_name(backend_context, config->local_name);
    backend->set_default_link_policy_settings(backend_context, config->link_policy_settings);
    backend->set_allow_role_switch(backend_context, config->allow_role_switch);
    return SWBT_BTSTACK_CLASSIC_DISCOVERY_OK;
}
