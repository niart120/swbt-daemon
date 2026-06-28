#include "support/swbt_version.h"

#ifndef SWBT_VERSION_STRING
#define SWBT_VERSION_STRING "0.1.0-dev"
#endif

const char *swbt_get_version_string(void) {
    return SWBT_VERSION_STRING;
}
