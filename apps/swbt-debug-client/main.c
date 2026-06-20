#include <stdio.h>

#include "debug_client.h"

int main(int argc, char **argv) {
    return swbt_debug_client_main(argc, (const char *const *)argv, stdout, stderr);
}
