#ifndef SWBT_BTSTACK_BRIDGE_CLASSIC_DISCOVERY_H
#define SWBT_BTSTACK_BRIDGE_CLASSIC_DISCOVERY_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SWBT_BTSTACK_CLASSIC_DISCOVERY_OK = 0,
    SWBT_BTSTACK_CLASSIC_DISCOVERY_ERROR_INVALID_ARGUMENT = -1,
} swbt_btstack_classic_discovery_result_t;

typedef struct {
    uint32_t class_of_device;
    const char *local_name;
    uint16_t link_policy_settings;
    bool allow_role_switch;
    bool discoverable;
} swbt_btstack_classic_discovery_config_t;

typedef struct {
    void (*discoverable_control)(void *context, uint8_t enable);
    void (*set_class_of_device)(void *context, uint32_t class_of_device);
    void (*set_local_name)(void *context, const char *local_name);
    void (*set_default_link_policy_settings)(void *context, uint16_t settings);
    void (*set_allow_role_switch)(void *context, bool allow_role_switch);
} swbt_btstack_classic_discovery_backend_t;

swbt_btstack_classic_discovery_result_t
swbt_btstack_classic_discovery_configure(const swbt_btstack_classic_discovery_backend_t *backend,
                                         void *backend_context,
                                         const swbt_btstack_classic_discovery_config_t *config);

#endif
