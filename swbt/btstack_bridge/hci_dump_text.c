#include "btstack_bridge/hci_dump_text.h"

#include <stdarg.h>
#include <stdio.h>

static FILE *g_swbt_btstack_hci_dump_text_file;

static void swbt_btstack_hci_dump_text_reset(void) {
    if (g_swbt_btstack_hci_dump_text_file != NULL) {
        fflush(g_swbt_btstack_hci_dump_text_file);
    }
}

static void swbt_btstack_hci_dump_text_log_packet(uint8_t packet_type, uint8_t in, uint8_t *packet,
                                                  uint16_t len) {
    if (g_swbt_btstack_hci_dump_text_file == NULL) {
        return;
    }

    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    fprintf(g_swbt_btstack_hci_dump_text_file,
            "packet type=0x%02x in=%u len=%u data=", (unsigned int)packet_type, (unsigned int)in,
            (unsigned int)len);
    for (uint16_t index = 0; index < len; index += 1u) {
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(g_swbt_btstack_hci_dump_text_file, "%02x", (unsigned int)packet[index]);
    }
    fputc('\n', g_swbt_btstack_hci_dump_text_file);
    fflush(g_swbt_btstack_hci_dump_text_file);
}

static void swbt_btstack_hci_dump_text_log_message(int log_level, const char *format,
                                                   va_list argptr) {
    if (g_swbt_btstack_hci_dump_text_file == NULL) {
        return;
    }

    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    fprintf(g_swbt_btstack_hci_dump_text_file, "log level=%d message=", log_level);
    vfprintf(g_swbt_btstack_hci_dump_text_file, format, argptr);
    fputc('\n', g_swbt_btstack_hci_dump_text_file);
    fflush(g_swbt_btstack_hci_dump_text_file);
}

swbt_btstack_hci_dump_text_result_t swbt_btstack_hci_dump_text_open(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return SWBT_BTSTACK_HCI_DUMP_TEXT_ERROR_INVALID_ARGUMENT;
    }

    swbt_btstack_hci_dump_text_close();
    g_swbt_btstack_hci_dump_text_file = fopen(path, "wb");
    if (g_swbt_btstack_hci_dump_text_file == NULL) {
        return SWBT_BTSTACK_HCI_DUMP_TEXT_ERROR_OPEN_FAILED;
    }
    fputs("swbt hci dump text v1\n", g_swbt_btstack_hci_dump_text_file);
    fflush(g_swbt_btstack_hci_dump_text_file);
    return SWBT_BTSTACK_HCI_DUMP_TEXT_OK;
}

void swbt_btstack_hci_dump_text_close(void) {
    if (g_swbt_btstack_hci_dump_text_file != NULL) {
        fclose(g_swbt_btstack_hci_dump_text_file);
        g_swbt_btstack_hci_dump_text_file = NULL;
    }
}

const hci_dump_t *swbt_btstack_hci_dump_text_instance(void) {
    static const hci_dump_t dump = {
        .reset = swbt_btstack_hci_dump_text_reset,
        .log_packet = swbt_btstack_hci_dump_text_log_packet,
        .log_message = swbt_btstack_hci_dump_text_log_message,
    };
    return &dump;
}
