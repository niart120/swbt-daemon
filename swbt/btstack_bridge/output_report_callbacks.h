#ifndef SWBT_BTSTACK_BRIDGE_OUTPUT_REPORT_CALLBACKS_H
#define SWBT_BTSTACK_BRIDGE_OUTPUT_REPORT_CALLBACKS_H

#include "btstack_bridge/output_report_handler.h"

typedef enum {
    SWBT_BTSTACK_OUTPUT_REPORT_CALLBACKS_OK = 0,
    SWBT_BTSTACK_OUTPUT_REPORT_CALLBACKS_ERROR_INVALID_ARGUMENT = -1,
} swbt_btstack_output_report_callbacks_result_t;

swbt_btstack_output_report_callbacks_result_t
swbt_btstack_output_report_callbacks_register(swbt_btstack_output_report_handler_t *handler);

void swbt_btstack_output_report_callbacks_unregister(void);

#endif
