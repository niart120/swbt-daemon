#include "btstack_bridge/output_report_callbacks.h"

#include <stddef.h>

#include "classic/hid_device.h"

static swbt_btstack_output_report_handler_t *g_output_report_handler;

// NOLINTBEGIN(bugprone-easily-swappable-parameters): BTstack HID callback ABI.
static void swbt_btstack_output_report_callbacks_dispatch(uint16_t hid_cid,
                                                          hid_report_type_t report_type,
                                                          uint16_t report_id, int report_size,
                                                          uint8_t *report) {
    if (g_output_report_handler == NULL || report_size < 0) {
        return;
    }

    (void)swbt_btstack_output_report_handler_handle(g_output_report_handler,
                                                    (swbt_btstack_output_report_handle_options_t){
                                                        .hid_cid = hid_cid,
                                                        .report_type = (uint8_t)report_type,
                                                        .report_id = report_id,
                                                        .report = report,
                                                        .report_size = (size_t)report_size,
                                                    });
}

static void swbt_btstack_output_report_data_callback(uint16_t hid_cid,
                                                     hid_report_type_t report_type,
                                                     uint16_t report_id, int report_size,
                                                     uint8_t *report) {
    swbt_btstack_output_report_callbacks_dispatch(hid_cid, report_type, report_id, report_size,
                                                  report);
}

static void swbt_btstack_set_report_callback(uint16_t hid_cid, hid_report_type_t report_type,
                                             int report_size, uint8_t *report) {
    swbt_btstack_output_report_callbacks_dispatch(hid_cid, report_type, 0u, report_size, report);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

swbt_btstack_output_report_callbacks_result_t
swbt_btstack_output_report_callbacks_register(swbt_btstack_output_report_handler_t *handler) {
    if (handler == NULL) {
        return SWBT_BTSTACK_OUTPUT_REPORT_CALLBACKS_ERROR_INVALID_ARGUMENT;
    }

    g_output_report_handler = handler;
    hid_device_register_report_data_callback(swbt_btstack_output_report_data_callback);
    hid_device_register_set_report_callback(swbt_btstack_set_report_callback);
    return SWBT_BTSTACK_OUTPUT_REPORT_CALLBACKS_OK;
}

void swbt_btstack_output_report_callbacks_unregister(void) {
    g_output_report_handler = NULL;
    hid_device_register_report_data_callback(NULL);
    hid_device_register_set_report_callback(NULL);
}
