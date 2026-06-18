#ifndef SWBT_BTSTACK_BRIDGE_OUTPUT_REPORT_HANDLER_H
#define SWBT_BTSTACK_BRIDGE_OUTPUT_REPORT_HANDLER_H

#include <stddef.h>
#include <stdint.h>

#include "switch/switch_subcommand.h"

#define SWBT_BTSTACK_HID_REPORT_TYPE_INPUT 1u
#define SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT 2u
#define SWBT_BTSTACK_OUTPUT_REPORT_MAX_SIZE 64u

typedef enum {
    SWBT_BTSTACK_OUTPUT_REPORT_OK = 0,
    SWBT_BTSTACK_OUTPUT_REPORT_IGNORED_REPORT_TYPE = 1,
    SWBT_BTSTACK_OUTPUT_REPORT_ERROR_INVALID_ARGUMENT = -1,
    SWBT_BTSTACK_OUTPUT_REPORT_ERROR_REPORT_ID_TOO_LARGE = -2,
    SWBT_BTSTACK_OUTPUT_REPORT_ERROR_BUFFER_TOO_SMALL = -3,
    SWBT_BTSTACK_OUTPUT_REPORT_ERROR_PARSE_FAILED = -4,
} swbt_btstack_output_report_result_t;

typedef void (*swbt_btstack_output_report_callback_t)(void *context, uint16_t hid_cid,
                                                      const swbt_switch_output_report_t *report);

typedef struct {
    swbt_btstack_output_report_callback_t callback;
    void *callback_context;
    uint8_t scratch[SWBT_BTSTACK_OUTPUT_REPORT_MAX_SIZE];
} swbt_btstack_output_report_handler_t;

void swbt_btstack_output_report_handler_init(swbt_btstack_output_report_handler_t *handler,
                                             swbt_btstack_output_report_callback_t callback,
                                             void *callback_context);

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_btstack_output_report_result_t
swbt_btstack_output_report_handler_handle(swbt_btstack_output_report_handler_t *handler,
                                          uint16_t hid_cid, uint8_t report_type, uint16_t report_id,
                                          const uint8_t *report, size_t report_size);
// NOLINTEND(bugprone-easily-swappable-parameters)

#endif
