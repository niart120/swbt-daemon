#include "daemon/switch_address.h"

#include <string.h>

static int expect_true(bool actual, const char *label) {
    (void)label;
    return actual ? 0 : 1;
}

static int expect_str_eq(const char *actual, const char *expected, const char *label) {
    (void)label;
    return actual != NULL && expected != NULL && strcmp(actual, expected) == 0 ? 0 : 1;
}

static int lowercase_input_is_normalized_to_uppercase_colon_separated_text(void) {
    char text[SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE];
    int failed = 0;
    failed +=
        expect_true(swbt_daemon_switch_address_normalize(text, "01:23:45:67:89:ab"), "normalize");
    failed += expect_str_eq(text, "01:23:45:67:89:AB", "normalized text");
    return failed;
}

int main(void) {
    int failed = 0;
    failed += lowercase_input_is_normalized_to_uppercase_colon_separated_text();
    return failed == 0 ? 0 : 1;
}
