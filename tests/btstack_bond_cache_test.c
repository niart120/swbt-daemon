#include "btstack_bridge/bond_cache.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bluetooth.h"
#include "btstack_tlv.h"
#include "classic/btstack_link_key_db.h"

enum {
    FAKE_TLV_MAX_TAGS = 8,
    FAKE_TLV_MAX_VALUE_SIZE = 64,
};

typedef struct {
    bool present;
    uint32_t tag;
    uint8_t value[FAKE_TLV_MAX_VALUE_SIZE];
    uint32_t value_size;
} fake_tlv_entry_t;

typedef struct {
    fake_tlv_entry_t entries[FAKE_TLV_MAX_TAGS];
} fake_tlv_t;

typedef struct {
    fake_tlv_t tlv;
    int init_tlv_calls;
    int set_tlv_instance_calls;
    int set_link_key_db_calls;
    int remove_path_calls;
    int remove_path_result;
    char init_path[96];
    char removed_path[96];
    const btstack_tlv_t *captured_tlv_impl;
    void *captured_tlv_context;
    const btstack_link_key_db_t *captured_link_key_db;
} fake_bond_platform_t;

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static fake_tlv_entry_t *fake_find_entry(fake_tlv_t *tlv, uint32_t tag) {
    for (size_t index = 0; index < FAKE_TLV_MAX_TAGS; ++index) {
        if (tlv->entries[index].present && tlv->entries[index].tag == tag) {
            return &tlv->entries[index];
        }
    }
    return NULL;
}

static fake_tlv_entry_t *fake_find_or_create_entry(fake_tlv_t *tlv, uint32_t tag) {
    fake_tlv_entry_t *entry = fake_find_entry(tlv, tag);
    if (entry != NULL) {
        return entry;
    }

    for (size_t index = 0; index < FAKE_TLV_MAX_TAGS; ++index) {
        if (!tlv->entries[index].present) {
            tlv->entries[index].present = true;
            tlv->entries[index].tag = tag;
            return &tlv->entries[index];
        }
    }
    return NULL;
}

static int fake_get_tag(void *context, uint32_t tag, uint8_t *buffer, uint32_t buffer_size) {
    fake_tlv_entry_t *entry = fake_find_entry((fake_tlv_t *)context, tag);
    if (entry == NULL || entry->value_size > buffer_size) {
        return 0;
    }
    (void)memcpy(buffer, entry->value, entry->value_size);
    return (int)entry->value_size;
}

static int fake_store_tag(void *context, uint32_t tag, const uint8_t *data, uint32_t data_size) {
    fake_tlv_entry_t *entry = fake_find_or_create_entry((fake_tlv_t *)context, tag);
    if (entry == NULL || data == NULL || data_size > FAKE_TLV_MAX_VALUE_SIZE) {
        return -1;
    }
    (void)memcpy(entry->value, data, data_size);
    entry->value_size = data_size;
    return 0;
}

static void fake_delete_tag(void *context, uint32_t tag) {
    fake_tlv_entry_t *entry = fake_find_entry((fake_tlv_t *)context, tag);
    if (entry != NULL) {
        *entry = (fake_tlv_entry_t){0};
    }
}

static const btstack_tlv_t fake_tlv_impl = {
    .get_tag = fake_get_tag,
    .store_tag = fake_store_tag,
    .delete_tag = fake_delete_tag,
};

static const btstack_tlv_t *fake_init_tlv(void *context, void *tlv_context, const char *path) {
    fake_bond_platform_t *platform = (fake_bond_platform_t *)context;
    platform->init_tlv_calls += 1;
    if (path != NULL) {
        (void)snprintf(platform->init_path, sizeof(platform->init_path), "%s", path);
    }
    return tlv_context == &platform->tlv ? &fake_tlv_impl : NULL;
}

static void fake_set_tlv_instance(void *context, const btstack_tlv_t *tlv_impl, void *tlv_context) {
    fake_bond_platform_t *platform = (fake_bond_platform_t *)context;
    platform->set_tlv_instance_calls += 1;
    platform->captured_tlv_impl = tlv_impl;
    platform->captured_tlv_context = tlv_context;
}

static void fake_set_link_key_db(void *context, const btstack_link_key_db_t *link_key_db) {
    fake_bond_platform_t *platform = (fake_bond_platform_t *)context;
    platform->set_link_key_db_calls += 1;
    platform->captured_link_key_db = link_key_db;
}

static int fake_remove_path(void *context, const char *path) {
    fake_bond_platform_t *platform = (fake_bond_platform_t *)context;
    platform->remove_path_calls += 1;
    if (path != NULL) {
        (void)snprintf(platform->removed_path, sizeof(platform->removed_path), "%s", path);
    }
    return platform->remove_path_result;
}

static swbt_btstack_bond_cache_platform_t fake_bond_platform(void) {
    return (swbt_btstack_bond_cache_platform_t){
        .init_tlv = fake_init_tlv,
        .set_tlv_instance = fake_set_tlv_instance,
        .set_link_key_db = fake_set_link_key_db,
    };
}

static int stores_reloads_and_deletes_link_key_in_tlv_backend(void) {
    fake_tlv_t tlv = {0};
    const bd_addr_t address = {0x98u, 0x76u, 0x54u, 0x32u, 0x10u, 0x01u};
    const link_key_t stored_key = {0x00u, 0x11u, 0x22u, 0x33u, 0x44u, 0x55u, 0x66u, 0x77u,
                                   0x88u, 0x99u, 0xaau, 0xbbu, 0xccu, 0xddu, 0xeeu, 0xffu};
    link_key_t loaded_key = {0};
    link_key_type_t loaded_type = INVALID_LINK_KEY;
    const btstack_link_key_db_t *db =
        swbt_btstack_bond_cache_link_key_db_from_tlv(&fake_tlv_impl, &tlv);

    int failed = 0;
    failed += expect_true(db != NULL);
    db->put_link_key((uint8_t *)address, (uint8_t *)stored_key,
                     AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256);

    const btstack_link_key_db_t *reloaded_db =
        swbt_btstack_bond_cache_link_key_db_from_tlv(&fake_tlv_impl, &tlv);
    failed += expect_true(reloaded_db != NULL);
    failed +=
        expect_eq_int(reloaded_db->get_link_key((uint8_t *)address, loaded_key, &loaded_type), 1);
    failed += expect_eq_int(loaded_type, AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256);
    for (size_t index = 0; index < LINK_KEY_LEN; ++index) {
        failed += expect_eq_u8(loaded_key[index], stored_key[index]);
    }

    reloaded_db->delete_link_key((uint8_t *)address);
    failed +=
        expect_eq_int(reloaded_db->get_link_key((uint8_t *)address, loaded_key, &loaded_type), 0);
    return failed;
}

static int invalid_tlv_arguments_are_rejected(void) {
    fake_tlv_t tlv = {0};
    int failed = 0;
    failed += expect_true(swbt_btstack_bond_cache_link_key_db_from_tlv(NULL, &tlv) == NULL);
    failed +=
        expect_true(swbt_btstack_bond_cache_link_key_db_from_tlv(&fake_tlv_impl, NULL) == NULL);
    return failed;
}

static int configures_tlv_link_key_db_for_local_address(void) {
    fake_bond_platform_t platform = {0};
    char path_buffer[96];
    const swbt_btstack_bond_cache_platform_t bond_platform = fake_bond_platform();
    const uint8_t local_address[6] = {0x98u, 0x76u, 0x54u, 0x32u, 0x10u, 0x01u};
    const swbt_btstack_bond_cache_config_t config = {
        .platform = &bond_platform,
        .platform_context = &platform,
        .tlv_context = &platform.tlv,
        .path_buffer = path_buffer,
        .path_buffer_size = sizeof(path_buffer),
    };

    const swbt_btstack_bond_cache_result_t result =
        swbt_btstack_bond_cache_configure_for_local_address(&config, local_address);

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_BOND_CACHE_OK);
    failed += expect_eq_int(platform.init_tlv_calls, 1);
    failed += expect_eq_int(strcmp(platform.init_path, "swbt-bond-98-76-54-32-10-01.tlv"), 0);
    failed += expect_eq_int(strcmp(path_buffer, "swbt-bond-98-76-54-32-10-01.tlv"), 0);
    failed += expect_eq_int(platform.set_tlv_instance_calls, 1);
    failed += expect_true(platform.captured_tlv_impl == &fake_tlv_impl);
    failed += expect_true(platform.captured_tlv_context == &platform.tlv);
    failed += expect_eq_int(platform.set_link_key_db_calls, 1);
    failed += expect_true(platform.captured_link_key_db != NULL);
    return failed;
}

static int invalid_configure_arguments_are_rejected(void) {
    fake_bond_platform_t platform = {0};
    char path_buffer[96];
    const swbt_btstack_bond_cache_platform_t bond_platform = fake_bond_platform();
    const uint8_t local_address[6] = {0x98u, 0x76u, 0x54u, 0x32u, 0x10u, 0x01u};
    swbt_btstack_bond_cache_config_t config = {
        .platform = &bond_platform,
        .platform_context = &platform,
        .tlv_context = &platform.tlv,
        .path_buffer = path_buffer,
        .path_buffer_size = sizeof(path_buffer),
    };

    int failed = 0;
    failed +=
        expect_eq_int(swbt_btstack_bond_cache_configure_for_local_address(NULL, local_address),
                      SWBT_BTSTACK_BOND_CACHE_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_bond_cache_configure_for_local_address(&config, NULL),
                            SWBT_BTSTACK_BOND_CACHE_ERROR_INVALID_ARGUMENT);
    config.path_buffer_size = 8u;
    failed +=
        expect_eq_int(swbt_btstack_bond_cache_configure_for_local_address(&config, local_address),
                      SWBT_BTSTACK_BOND_CACHE_ERROR_BUFFER_TOO_SMALL);
    return failed;
}

static int cleanup_removes_local_address_bond_cache_without_env(void) {
    fake_bond_platform_t platform = {0};
    char path_buffer[96];
    const uint8_t local_address[6] = {0x98u, 0x76u, 0x54u, 0x32u, 0x10u, 0x01u};
    const swbt_btstack_bond_cache_cleanup_config_t config = {
        .remove_path = fake_remove_path,
        .remove_context = &platform,
        .path_buffer = path_buffer,
        .path_buffer_size = sizeof(path_buffer),
    };

    const swbt_btstack_bond_cache_result_t result =
        swbt_btstack_bond_cache_cleanup_for_local_address(&config, local_address);

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_BOND_CACHE_OK);
    failed += expect_eq_int(platform.remove_path_calls, 1);
    failed += expect_eq_int(strcmp(platform.removed_path, "swbt-bond-98-76-54-32-10-01.tlv"), 0);
    failed += expect_eq_int(strcmp(path_buffer, "swbt-bond-98-76-54-32-10-01.tlv"), 0);
    return failed;
}

static int invalid_cleanup_arguments_are_rejected(void) {
    fake_bond_platform_t platform = {0};
    char path_buffer[96];
    const uint8_t local_address[6] = {0x98u, 0x76u, 0x54u, 0x32u, 0x10u, 0x01u};
    swbt_btstack_bond_cache_cleanup_config_t config = {
        .remove_path = fake_remove_path,
        .remove_context = &platform,
        .path_buffer = path_buffer,
        .path_buffer_size = sizeof(path_buffer),
    };

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_bond_cache_cleanup_for_local_address(NULL, local_address),
                            SWBT_BTSTACK_BOND_CACHE_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_bond_cache_cleanup_for_local_address(&config, NULL),
                            SWBT_BTSTACK_BOND_CACHE_ERROR_INVALID_ARGUMENT);
    config.remove_path = NULL;
    failed +=
        expect_eq_int(swbt_btstack_bond_cache_cleanup_for_local_address(&config, local_address),
                      SWBT_BTSTACK_BOND_CACHE_ERROR_INVALID_ARGUMENT);
    config.remove_path = fake_remove_path;
    config.path_buffer_size = 8u;
    failed +=
        expect_eq_int(swbt_btstack_bond_cache_cleanup_for_local_address(&config, local_address),
                      SWBT_BTSTACK_BOND_CACHE_ERROR_BUFFER_TOO_SMALL);
    config.path_buffer_size = sizeof(path_buffer);
    platform.remove_path_result = -1;
    failed +=
        expect_eq_int(swbt_btstack_bond_cache_cleanup_for_local_address(&config, local_address),
                      SWBT_BTSTACK_BOND_CACHE_ERROR_RUNTIME);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += stores_reloads_and_deletes_link_key_in_tlv_backend();
    failed += invalid_tlv_arguments_are_rejected();
    failed += configures_tlv_link_key_db_for_local_address();
    failed += invalid_configure_arguments_are_rejected();
    failed += cleanup_removes_local_address_bond_cache_without_env();
    failed += invalid_cleanup_arguments_are_rejected();
    return failed == 0 ? 0 : 1;
}
