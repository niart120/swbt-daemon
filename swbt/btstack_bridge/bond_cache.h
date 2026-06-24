#ifndef SWBT_BTSTACK_BRIDGE_BOND_CACHE_H
#define SWBT_BTSTACK_BRIDGE_BOND_CACHE_H

#include <stddef.h>
#include <stdint.h>

#include "btstack_tlv.h"
#include "classic/btstack_link_key_db.h"

typedef enum {
    SWBT_BTSTACK_BOND_CACHE_OK = 0,
    SWBT_BTSTACK_BOND_CACHE_ERROR_INVALID_ARGUMENT = -1,
    SWBT_BTSTACK_BOND_CACHE_ERROR_BUFFER_TOO_SMALL = -2,
    SWBT_BTSTACK_BOND_CACHE_ERROR_RUNTIME = -3,
} swbt_btstack_bond_cache_result_t;

typedef struct {
    const btstack_tlv_t *(*init_tlv)(void *context, void *tlv_context, const char *path);
    void (*set_tlv_instance)(void *context, const btstack_tlv_t *tlv_impl, void *tlv_context);
    void (*set_link_key_db)(void *context, const btstack_link_key_db_t *link_key_db);
    void (*deinit_tlv)(void *context, void *tlv_context);
} swbt_btstack_bond_cache_platform_t;

typedef struct {
    const swbt_btstack_bond_cache_platform_t *platform;
    void *platform_context;
    void *tlv_context;
    char *path_buffer;
    size_t path_buffer_size;
} swbt_btstack_bond_cache_config_t;

typedef int (*swbt_btstack_bond_cache_remove_path_t)(void *context, const char *path);

typedef struct {
    swbt_btstack_bond_cache_remove_path_t remove_path;
    void *remove_context;
    char *path_buffer;
    size_t path_buffer_size;
} swbt_btstack_bond_cache_cleanup_config_t;

const btstack_link_key_db_t *
swbt_btstack_bond_cache_link_key_db_from_tlv(const btstack_tlv_t *tlv_impl, void *tlv_context);

swbt_btstack_bond_cache_result_t
swbt_btstack_bond_cache_configure_for_local_address(const swbt_btstack_bond_cache_config_t *config,
                                                    const uint8_t local_address[6]);

swbt_btstack_bond_cache_result_t swbt_btstack_bond_cache_cleanup_for_local_address(
    const swbt_btstack_bond_cache_cleanup_config_t *config, const uint8_t local_address[6]);

void swbt_btstack_bond_cache_deinit(const swbt_btstack_bond_cache_config_t *config);

#endif
