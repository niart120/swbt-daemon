#include "btstack_bridge/classic_discovery.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    STEP_DISCOVERABLE = 1,
    STEP_CLASS_OF_DEVICE,
    STEP_LOCAL_NAME,
    STEP_LINK_POLICY,
    STEP_ROLE_SWITCH,
} step_t;

typedef struct {
    step_t steps[8];
    size_t step_count;
    uint8_t discoverable;
    uint32_t class_of_device;
    const char *local_name;
    uint16_t link_policy_settings;
    bool allow_role_switch;
} fake_backend_t;

static void record_step(fake_backend_t *fake, step_t step) {
    if (fake->step_count < sizeof(fake->steps) / sizeof(fake->steps[0])) {
        fake->steps[fake->step_count] = step;
    }
    fake->step_count += 1u;
}

static void fake_discoverable_control(void *context, uint8_t enable) {
    fake_backend_t *fake = context;
    record_step(fake, STEP_DISCOVERABLE);
    fake->discoverable = enable;
}

static void fake_set_class_of_device(void *context, uint32_t class_of_device) {
    fake_backend_t *fake = context;
    record_step(fake, STEP_CLASS_OF_DEVICE);
    fake->class_of_device = class_of_device;
}

static void fake_set_local_name(void *context, const char *local_name) {
    fake_backend_t *fake = context;
    record_step(fake, STEP_LOCAL_NAME);
    fake->local_name = local_name;
}

static void fake_set_default_link_policy_settings(void *context, uint16_t settings) {
    fake_backend_t *fake = context;
    record_step(fake, STEP_LINK_POLICY);
    fake->link_policy_settings = settings;
}

static void fake_set_allow_role_switch(void *context, bool allow_role_switch) {
    fake_backend_t *fake = context;
    record_step(fake, STEP_ROLE_SWITCH);
    fake->allow_role_switch = allow_role_switch;
}

static swbt_btstack_classic_discovery_backend_t fake_ops(void) {
    return (swbt_btstack_classic_discovery_backend_t){
        .discoverable_control = fake_discoverable_control,
        .set_class_of_device = fake_set_class_of_device,
        .set_local_name = fake_set_local_name,
        .set_default_link_policy_settings = fake_set_default_link_policy_settings,
        .set_allow_role_switch = fake_set_allow_role_switch,
    };
}

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int configures_gap_settings_before_power_on(void) {
    fake_backend_t fake = {0};
    const swbt_btstack_classic_discovery_backend_t backend = fake_ops();
    const swbt_btstack_classic_discovery_config_t config = {
        .class_of_device = 0x2508u,
        .local_name = "Pro Controller",
        .link_policy_settings = 0x0005u,
        .allow_role_switch = true,
        .discoverable = true,
    };
    const step_t expected_steps[] = {
        STEP_DISCOVERABLE, STEP_CLASS_OF_DEVICE, STEP_LOCAL_NAME,
        STEP_LINK_POLICY,  STEP_ROLE_SWITCH,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_classic_discovery_configure(&backend, &fake, &config),
                            SWBT_BTSTACK_CLASSIC_DISCOVERY_OK);
    failed += expect_eq_int((int)fake.step_count,
                            (int)(sizeof(expected_steps) / sizeof(expected_steps[0])));
    for (size_t i = 0; i < sizeof(expected_steps) / sizeof(expected_steps[0]); i += 1u) {
        failed += expect_eq_int((int)fake.steps[i], (int)expected_steps[i]);
    }
    failed += expect_eq_u8(fake.discoverable, 1u);
    failed += expect_eq_u32(fake.class_of_device, 0x2508u);
    failed += expect_true(fake.local_name == config.local_name);
    failed += expect_eq_u16(fake.link_policy_settings, 0x0005u);
    failed += expect_true(fake.allow_role_switch);
    return failed;
}

static int rejects_missing_local_name(void) {
    fake_backend_t fake = {0};
    const swbt_btstack_classic_discovery_backend_t backend = fake_ops();
    const swbt_btstack_classic_discovery_config_t config = {
        .class_of_device = 0x2508u,
        .local_name = NULL,
        .link_policy_settings = 0x0005u,
        .allow_role_switch = true,
        .discoverable = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_classic_discovery_configure(&backend, &fake, &config),
                            SWBT_BTSTACK_CLASSIC_DISCOVERY_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int((int)fake.step_count, 0);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += configures_gap_settings_before_power_on();
    failed += rejects_missing_local_name();
    return failed == 0 ? 0 : 1;
}
