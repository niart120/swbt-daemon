#include "btstack_bridge/bond_cache.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "classic/btstack_link_key_db_tlv.h"

static bool
swbt_btstack_bond_cache_platform_is_valid(const swbt_btstack_bond_cache_platform_t *platform) {
    return platform != NULL && platform->init_tlv != NULL && platform->set_tlv_instance != NULL &&
           platform->set_link_key_db != NULL;
}

static bool
swbt_btstack_bond_cache_config_is_valid(const swbt_btstack_bond_cache_config_t *config) {
    return config != NULL && swbt_btstack_bond_cache_platform_is_valid(config->platform) &&
           config->tlv_context != NULL && config->path_buffer != NULL &&
           config->path_buffer_size > 0u;
}

static bool swbt_btstack_bond_cache_tlv_is_valid(const btstack_tlv_t *tlv_impl,
                                                 const void *tlv_context) {
    return tlv_impl != NULL && tlv_context != NULL && tlv_impl->get_tag != NULL &&
           tlv_impl->store_tag != NULL && tlv_impl->delete_tag != NULL;
}

const btstack_link_key_db_t *
swbt_btstack_bond_cache_link_key_db_from_tlv(const btstack_tlv_t *tlv_impl, void *tlv_context) {
    if (!swbt_btstack_bond_cache_tlv_is_valid(tlv_impl, tlv_context)) {
        return NULL;
    }
    return btstack_link_key_db_tlv_get_instance(tlv_impl, tlv_context);
}

static swbt_btstack_bond_cache_result_t
swbt_btstack_bond_cache_format_path(char *buffer, size_t buffer_size,
                                    const uint8_t local_address[6]) {
    const int written = snprintf(buffer, buffer_size, "swbt-bond-%02x-%02x-%02x-%02x-%02x-%02x.tlv",
                                 (unsigned int)local_address[0], (unsigned int)local_address[1],
                                 (unsigned int)local_address[2], (unsigned int)local_address[3],
                                 (unsigned int)local_address[4], (unsigned int)local_address[5]);

    if (written < 0 || (size_t)written >= buffer_size) {
        return SWBT_BTSTACK_BOND_CACHE_ERROR_BUFFER_TOO_SMALL;
    }
    return SWBT_BTSTACK_BOND_CACHE_OK;
}

swbt_btstack_bond_cache_result_t
swbt_btstack_bond_cache_configure_for_local_address(const swbt_btstack_bond_cache_config_t *config,
                                                    const uint8_t local_address[6]) {
    if (!swbt_btstack_bond_cache_config_is_valid(config) || local_address == NULL) {
        return SWBT_BTSTACK_BOND_CACHE_ERROR_INVALID_ARGUMENT;
    }

    const swbt_btstack_bond_cache_result_t path_result = swbt_btstack_bond_cache_format_path(
        config->path_buffer, config->path_buffer_size, local_address);
    if (path_result != SWBT_BTSTACK_BOND_CACHE_OK) {
        return path_result;
    }

    const btstack_tlv_t *tlv_impl = config->platform->init_tlv(
        config->platform_context, config->tlv_context, config->path_buffer);
    if (!swbt_btstack_bond_cache_tlv_is_valid(tlv_impl, config->tlv_context)) {
        return SWBT_BTSTACK_BOND_CACHE_ERROR_RUNTIME;
    }

    const btstack_link_key_db_t *link_key_db =
        swbt_btstack_bond_cache_link_key_db_from_tlv(tlv_impl, config->tlv_context);
    if (link_key_db == NULL) {
        return SWBT_BTSTACK_BOND_CACHE_ERROR_RUNTIME;
    }

    config->platform->set_tlv_instance(config->platform_context, tlv_impl, config->tlv_context);
    config->platform->set_link_key_db(config->platform_context, link_key_db);
    return SWBT_BTSTACK_BOND_CACHE_OK;
}

void swbt_btstack_bond_cache_deinit(const swbt_btstack_bond_cache_config_t *config) {
    if (config == NULL || config->platform == NULL || config->platform->deinit_tlv == NULL ||
        config->tlv_context == NULL) {
        return;
    }
    config->platform->deinit_tlv(config->platform_context, config->tlv_context);
}
