#include "btstack_bridge/bond_cache.h"

#include <stdbool.h>
#include <stddef.h>

#include "classic/btstack_link_key_db_tlv.h"

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
