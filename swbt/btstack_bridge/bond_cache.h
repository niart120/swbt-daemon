#ifndef SWBT_BTSTACK_BRIDGE_BOND_CACHE_H
#define SWBT_BTSTACK_BRIDGE_BOND_CACHE_H

#include "btstack_tlv.h"
#include "classic/btstack_link_key_db.h"

const btstack_link_key_db_t *
swbt_btstack_bond_cache_link_key_db_from_tlv(const btstack_tlv_t *tlv_impl, void *tlv_context);

#endif
