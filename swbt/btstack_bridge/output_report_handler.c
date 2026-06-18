#include "btstack_bridge/output_report_handler.h"

#include <stdbool.h>
#include <stdint.h>

void swbt_btstack_output_report_handler_init(swbt_btstack_output_report_handler_t *handler,
                                             swbt_btstack_output_report_callback_t callback,
                                             void *callback_context) {
    if (handler == NULL) {
        return;
    }

    handler->callback = callback;
    handler->callback_context = callback_context;
}

static bool
swbt_btstack_output_report_handler_is_valid(const swbt_btstack_output_report_handler_t *handler) {
    return handler != NULL && handler->callback != NULL;
}

static swbt_btstack_output_report_result_t
swbt_btstack_output_report_rejoin_id(swbt_btstack_output_report_handler_t *handler,
                                     uint16_t report_id, const uint8_t *report, size_t report_size,
                                     const uint8_t **out_report, size_t *out_report_size) {
    if (report_id > UINT8_MAX) {
        return SWBT_BTSTACK_OUTPUT_REPORT_ERROR_REPORT_ID_TOO_LARGE;
    }
    if (report_size >= SWBT_BTSTACK_OUTPUT_REPORT_MAX_SIZE) {
        return SWBT_BTSTACK_OUTPUT_REPORT_ERROR_BUFFER_TOO_SMALL;
    }

    handler->scratch[0] = (uint8_t)report_id;
    for (size_t index = 0; index < report_size; ++index) {
        handler->scratch[index + 1u] = report[index];
    }

    *out_report = handler->scratch;
    *out_report_size = report_size + 1u;
    return SWBT_BTSTACK_OUTPUT_REPORT_OK;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_btstack_output_report_result_t
swbt_btstack_output_report_handler_handle(swbt_btstack_output_report_handler_t *handler,
                                          uint16_t hid_cid, uint8_t report_type, uint16_t report_id,
                                          const uint8_t *report, size_t report_size) {
    if (!swbt_btstack_output_report_handler_is_valid(handler) || report == NULL) {
        return SWBT_BTSTACK_OUTPUT_REPORT_ERROR_INVALID_ARGUMENT;
    }
    if (report_type != SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT) {
        return SWBT_BTSTACK_OUTPUT_REPORT_IGNORED_REPORT_TYPE;
    }

    const uint8_t *parser_report = report;
    size_t parser_report_size = report_size;
    if (report_id != 0u) {
        const swbt_btstack_output_report_result_t rejoin_result =
            swbt_btstack_output_report_rejoin_id(handler, report_id, report, report_size,
                                                 &parser_report, &parser_report_size);
        if (rejoin_result != SWBT_BTSTACK_OUTPUT_REPORT_OK) {
            return rejoin_result;
        }
    }

    swbt_switch_output_report_t parsed;
    if (swbt_switch_parse_output_report(parser_report, parser_report_size, &parsed) !=
        SWBT_SWITCH_SUBCOMMAND_OK) {
        return SWBT_BTSTACK_OUTPUT_REPORT_ERROR_PARSE_FAILED;
    }

    handler->callback(handler->callback_context, hid_cid, &parsed);
    return SWBT_BTSTACK_OUTPUT_REPORT_OK;
}
// NOLINTEND(bugprone-easily-swappable-parameters)
