#include <stddef.h>
#include <stdint.h>

#include "switch/switch_controller_state.h"
#include "switch/switch_device_info.h"
#include "switch/switch_player_lights.h"
#include "switch/switch_report.h"
#include "switch/switch_spi.h"
#include "switch/switch_subcommand.h"
#include "switch/switch_subcommand_dispatcher.h"
#include "switch/switch_subcommand_reply.h"

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_size(size_t actual, size_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_range(const uint8_t *actual, const uint8_t *expected, size_t size) {
    for (size_t index = 0; index < size; ++index) {
        if (actual[index] != expected[index]) {
            return 1;
        }
    }
    return 0;
}

static int expect_eq_action(swbt_switch_subcommand_dispatch_action_t actual,
                            swbt_switch_subcommand_dispatch_action_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_zero_range(const uint8_t *actual, size_t start, size_t end) {
    for (size_t index = start; index < end; ++index) {
        if (actual[index] != 0u) {
            return 1;
        }
    }
    return 0;
}

static swbt_state_t sample_state(void) {
    swbt_state_t state = swbt_state_neutral();
    state.buttons = 0x00010203u;
    state.lx = 0x0111u;
    state.ly = 0x0222u;
    state.rx = 0x0333u;
    state.ry = 0x0444u;
    return state;
}

static swbt_switch_report_options_t sample_report_options(void) {
    swbt_switch_report_options_t options = {
        .timer = 0x42u,
        .battery_connection = 0x8Eu,
        .vibrator_report = 0x80u,
    };
    return options;
}

static swbt_switch_subcommand_dispatcher_config_t
sample_config(const swbt_state_t *state, const swbt_switch_report_options_t *report_options,
              const swbt_switch_spi_t *spi) {
    swbt_switch_subcommand_dispatcher_config_t config = {
        .state = state,
        .report_options = report_options,
        .spi = spi,
    };
    return config;
}

static swbt_switch_subcommand_dispatcher_config_t
sample_config_with_device_info(const swbt_state_t *state,
                               const swbt_switch_report_options_t *report_options,
                               const swbt_switch_device_info_t *device_info) {
    swbt_switch_subcommand_dispatcher_config_t config = sample_config(state, report_options, NULL);
    config.device_info = device_info;
    return config;
}

static swbt_switch_subcommand_dispatcher_config_t
sample_config_with_player_lights(const swbt_state_t *state,
                                 const swbt_switch_report_options_t *report_options,
                                 swbt_switch_player_lights_state_t *player_lights) {
    swbt_switch_subcommand_dispatcher_config_t config = sample_config(state, report_options, NULL);
    config.player_lights = player_lights;
    return config;
}

static swbt_switch_output_report_t subcommand_report(uint8_t subcommand_id, const uint8_t *data,
                                                     size_t data_len) {
    swbt_switch_output_report_t report = {
        .output_report_id = SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND,
        .packet_counter = 0x0Au,
        .has_subcommand = true,
        .subcommand_id = subcommand_id,
        .subcommand_data = data,
        .subcommand_data_len = data_len,
    };
    return report;
}

static int test_simple_ack_dispatches_set_report_mode_reply(void) {
    const uint8_t data[] = {0x30u};
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    swbt_switch_subcommand_dispatcher_config_t config =
        sample_config(&state, &report_options, NULL);
    swbt_switch_output_report_t output =
        subcommand_report(SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE, data, sizeof(data));
    swbt_switch_subcommand_dispatcher_response_t response;

    int failed = 0;
    failed += expect_eq_int(swbt_switch_subcommand_dispatch(&config, &output, &response),
                            SWBT_SWITCH_SUBCOMMAND_DISPATCH_OK);
    failed += expect_eq_action(response.action, SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY);
    failed += expect_eq_size(response.report_size, SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    failed += expect_eq_u8(response.report[0], SWBT_SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY);
    failed += expect_eq_u8(response.report[13], SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SIMPLE);
    failed += expect_eq_u8(response.report[14], SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE);
    failed += expect_zero_range(response.report, SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET,
                                SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    return failed;
}

static int test_low_power_mode_dispatches_simple_ack_reply(void) {
    const uint8_t data[] = {0x00u};
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    swbt_switch_subcommand_dispatcher_config_t config =
        sample_config(&state, &report_options, NULL);
    swbt_switch_output_report_t output =
        subcommand_report(SWBT_SWITCH_SUBCOMMAND_LOW_POWER_MODE, data, sizeof(data));
    swbt_switch_subcommand_dispatcher_response_t response;

    int failed = 0;
    failed += expect_eq_int(swbt_switch_subcommand_dispatch(&config, &output, &response),
                            SWBT_SWITCH_SUBCOMMAND_DISPATCH_OK);
    failed += expect_eq_action(response.action, SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY);
    failed += expect_eq_size(response.report_size, SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    failed += expect_eq_u8(response.report[13], SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SIMPLE);
    failed += expect_eq_u8(response.report[14], SWBT_SWITCH_SUBCOMMAND_LOW_POWER_MODE);
    failed += expect_zero_range(response.report, SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET,
                                SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    return failed;
}

static int test_trigger_buttons_elapsed_time_builds_pairing_reply(void) {
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    const uint8_t expected_data[] = {0x2Cu, 0x01u, 0x2Cu, 0x01u, 0x00u, 0x00u, 0x00u,
                                     0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
    swbt_switch_subcommand_dispatcher_config_t config =
        sample_config(&state, &report_options, NULL);
    swbt_switch_output_report_t output =
        subcommand_report(SWBT_SWITCH_SUBCOMMAND_TRIGGER_BUTTONS_ELAPSED, NULL, 0u);
    swbt_switch_subcommand_dispatcher_response_t response;

    int failed = 0;
    failed += expect_eq_int(swbt_switch_subcommand_dispatch(&config, &output, &response),
                            SWBT_SWITCH_SUBCOMMAND_DISPATCH_OK);
    failed += expect_eq_action(response.action, SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY);
    failed += expect_eq_size(response.report_size, SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    failed += expect_eq_u8(response.report[13],
                           SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_TRIGGER_BUTTONS_ELAPSED);
    failed += expect_eq_u8(response.report[14], SWBT_SWITCH_SUBCOMMAND_TRIGGER_BUTTONS_ELAPSED);
    failed += expect_range(&response.report[SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET],
                           expected_data, sizeof(expected_data));
    failed += expect_zero_range(response.report,
                                SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET +
                                    sizeof(expected_data),
                                SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    return failed;
}

static int test_spi_flash_read_builds_reply_data_from_virtual_spi(void) {
    static swbt_switch_spi_t spi;
    const uint8_t device_type = SWBT_SWITCH_SPI_DEVICE_TYPE_PRO_CONTROLLER;
    const uint8_t data[] = {0x12u, 0x60u, 0x00u, 0x00u, 0x01u};
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    swbt_switch_subcommand_dispatcher_config_t config =
        sample_config(&state, &report_options, &spi);
    swbt_switch_output_report_t output =
        subcommand_report(SWBT_SWITCH_SUBCOMMAND_SPI_FLASH_READ, data, sizeof(data));
    swbt_switch_subcommand_dispatcher_response_t response;

    if (swbt_switch_spi_init(&spi) != SWBT_SWITCH_SPI_OK ||
        swbt_switch_spi_write(&spi, SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE, &device_type, 1u) !=
            SWBT_SWITCH_SPI_OK) {
        return 1;
    }

    int failed = 0;
    failed += expect_eq_int(swbt_switch_subcommand_dispatch(&config, &output, &response),
                            SWBT_SWITCH_SUBCOMMAND_DISPATCH_OK);
    failed += expect_eq_action(response.action, SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY);
    failed += expect_eq_u8(response.report[13], SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SPI_READ);
    failed += expect_eq_u8(response.report[14], SWBT_SWITCH_SUBCOMMAND_SPI_FLASH_READ);
    failed += expect_eq_u8(response.report[15], 0x12u);
    failed += expect_eq_u8(response.report[16], 0x60u);
    failed += expect_eq_u8(response.report[17], 0x00u);
    failed += expect_eq_u8(response.report[18], 0x00u);
    failed += expect_eq_u8(response.report[19], 0x01u);
    failed += expect_eq_u8(response.report[20], SWBT_SWITCH_SPI_DEVICE_TYPE_PRO_CONTROLLER);
    return failed;
}

static int test_set_player_lights_updates_state_and_builds_ack(void) {
    const uint8_t data[] = {0x53u};
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    swbt_switch_player_lights_state_t player_lights;
    swbt_switch_subcommand_dispatcher_config_t config =
        sample_config_with_player_lights(&state, &report_options, &player_lights);
    swbt_switch_output_report_t output =
        subcommand_report(SWBT_SWITCH_SUBCOMMAND_SET_PLAYER_LIGHTS, data, sizeof(data));
    swbt_switch_subcommand_dispatcher_response_t response;

    int failed = 0;
    failed +=
        expect_eq_int(swbt_switch_player_lights_init(&player_lights), SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_eq_int(swbt_switch_subcommand_dispatch(&config, &output, &response),
                            SWBT_SWITCH_SUBCOMMAND_DISPATCH_OK);
    failed += expect_eq_action(response.action, SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY);
    failed += expect_eq_u8(response.report[13], SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SIMPLE);
    failed += expect_eq_u8(response.report[14], SWBT_SWITCH_SUBCOMMAND_SET_PLAYER_LIGHTS);
    failed += expect_zero_range(response.report, SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET,
                                SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    failed += expect_eq_u8(player_lights.raw, 0x53u);
    failed += expect_eq_u8(player_lights.on_mask, 0x03u);
    failed += expect_eq_u8(player_lights.flash_mask, 0x05u);
    failed += expect_eq_u8(player_lights.effective_flash_mask, 0x04u);
    return failed;
}

static int test_get_player_lights_builds_current_state_reply(void) {
    const uint8_t data[] = {0x86u};
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    swbt_switch_player_lights_state_t player_lights;
    swbt_switch_subcommand_dispatcher_config_t config =
        sample_config_with_player_lights(&state, &report_options, &player_lights);
    swbt_switch_output_report_t output =
        subcommand_report(SWBT_SWITCH_SUBCOMMAND_GET_PLAYER_LIGHTS, NULL, 0u);
    swbt_switch_subcommand_dispatcher_response_t response;

    int failed = 0;
    failed +=
        expect_eq_int(swbt_switch_player_lights_init(&player_lights), SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_eq_int(swbt_switch_player_lights_apply_set(&player_lights, data, sizeof(data)),
                            SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_eq_int(swbt_switch_subcommand_dispatch(&config, &output, &response),
                            SWBT_SWITCH_SUBCOMMAND_DISPATCH_OK);
    failed += expect_eq_action(response.action, SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY);
    failed += expect_eq_u8(response.report[13], SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_PLAYER_LIGHTS);
    failed += expect_eq_u8(response.report[14], SWBT_SWITCH_SUBCOMMAND_GET_PLAYER_LIGHTS);
    failed += expect_eq_u8(response.report[15], 0x86u);
    failed += expect_zero_range(response.report, SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET + 1u,
                                SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    return failed;
}

static int test_malformed_player_lights_request_does_not_update_state(void) {
    const uint8_t data[] = {0x21u};
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    swbt_switch_player_lights_state_t player_lights;
    swbt_switch_subcommand_dispatcher_config_t config =
        sample_config_with_player_lights(&state, &report_options, &player_lights);
    swbt_switch_output_report_t output =
        subcommand_report(SWBT_SWITCH_SUBCOMMAND_SET_PLAYER_LIGHTS, NULL, 0u);
    swbt_switch_subcommand_dispatcher_response_t response;

    int failed = 0;
    failed +=
        expect_eq_int(swbt_switch_player_lights_init(&player_lights), SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_eq_int(swbt_switch_player_lights_apply_set(&player_lights, data, sizeof(data)),
                            SWBT_SWITCH_PLAYER_LIGHTS_OK);
    failed += expect_eq_int(swbt_switch_subcommand_dispatch(&config, &output, &response),
                            SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_MALFORMED_SUBCOMMAND);
    failed += expect_eq_action(response.action, SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_NONE);
    failed += expect_eq_size(response.report_size, 0u);
    failed += expect_eq_u8(player_lights.raw, 0x21u);
    return failed;
}

static int test_request_device_info_builds_pro_controller_identity_reply(void) {
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    swbt_switch_device_info_t device_info = swbt_switch_device_info_default();
    const uint8_t address[] = {0x00u, 0x1Bu, 0xDCu, 0xF9u, 0x9Fu, 0x7Du};
    const uint8_t expected_data[] = {0x04u, 0x00u, 0x03u, 0x02u, 0x00u, 0x1Bu,
                                     0xDCu, 0xF9u, 0x9Fu, 0x7Du, 0x01u, 0x01u};
    swbt_switch_subcommand_dispatcher_config_t config =
        sample_config_with_device_info(&state, &report_options, &device_info);
    swbt_switch_output_report_t output =
        subcommand_report(SWBT_SWITCH_SUBCOMMAND_REQUEST_DEVICE_INFO, NULL, 0u);
    swbt_switch_subcommand_dispatcher_response_t response;
    for (size_t index = 0; index < sizeof(address); ++index) {
        device_info.bluetooth_address[index] = address[index];
    }

    int failed = 0;
    failed += expect_eq_int(swbt_switch_subcommand_dispatch(&config, &output, &response),
                            SWBT_SWITCH_SUBCOMMAND_DISPATCH_OK);
    failed += expect_eq_action(response.action, SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY);
    failed += expect_eq_size(response.report_size, SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    failed += expect_eq_u8(response.report[13], SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_DEVICE_INFO);
    failed += expect_eq_u8(response.report[14], SWBT_SWITCH_SUBCOMMAND_REQUEST_DEVICE_INFO);
    failed += expect_range(&response.report[SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET],
                           expected_data, sizeof(expected_data));
    return failed;
}

static int test_request_device_info_accepts_mizuyoukanao_pro_profile(void) {
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    swbt_switch_device_info_t device_info = swbt_switch_device_info_mizuyoukanao_pro();
    const uint8_t address[] = {0x00u, 0x1Bu, 0xDCu, 0xF9u, 0x9Fu, 0x7Du};
    const uint8_t expected_data[] = {0x03u, 0x48u, 0x03u, 0x02u, 0x00u, 0x1Bu,
                                     0xDCu, 0xF9u, 0x9Fu, 0x7Du, 0x03u, 0x02u};
    swbt_switch_subcommand_dispatcher_config_t config =
        sample_config_with_device_info(&state, &report_options, &device_info);
    swbt_switch_output_report_t output =
        subcommand_report(SWBT_SWITCH_SUBCOMMAND_REQUEST_DEVICE_INFO, NULL, 0u);
    swbt_switch_subcommand_dispatcher_response_t response;
    for (size_t index = 0; index < sizeof(address); ++index) {
        device_info.bluetooth_address[index] = address[index];
    }

    int failed = 0;
    failed += expect_eq_int(swbt_switch_subcommand_dispatch(&config, &output, &response),
                            SWBT_SWITCH_SUBCOMMAND_DISPATCH_OK);
    failed += expect_eq_action(response.action, SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY);
    failed += expect_eq_size(response.report_size, SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    failed += expect_eq_u8(response.report[13], SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_DEVICE_INFO);
    failed += expect_eq_u8(response.report[14], SWBT_SWITCH_SUBCOMMAND_REQUEST_DEVICE_INFO);
    failed += expect_range(&response.report[SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET],
                           expected_data, sizeof(expected_data));
    return failed;
}

static int test_malformed_spi_request_does_not_build_reply(void) {
    static swbt_switch_spi_t spi;
    const uint8_t short_data[] = {0x12u, 0x60u, 0x00u, 0x00u};
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    swbt_switch_subcommand_dispatcher_config_t config =
        sample_config(&state, &report_options, &spi);
    swbt_switch_output_report_t output =
        subcommand_report(SWBT_SWITCH_SUBCOMMAND_SPI_FLASH_READ, short_data, sizeof(short_data));
    swbt_switch_subcommand_dispatcher_response_t response;

    if (swbt_switch_spi_init(&spi) != SWBT_SWITCH_SPI_OK) {
        return 1;
    }

    int failed = 0;
    failed += expect_eq_int(swbt_switch_subcommand_dispatch(&config, &output, &response),
                            SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_MALFORMED_SUBCOMMAND);
    failed += expect_eq_action(response.action, SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_NONE);
    failed += expect_eq_size(response.report_size, 0u);
    return failed;
}

static int test_rumble_only_report_has_no_reply_action(void) {
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    swbt_switch_subcommand_dispatcher_config_t config =
        sample_config(&state, &report_options, NULL);
    swbt_switch_output_report_t output = {
        .output_report_id = SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY,
        .packet_counter = 0x0Bu,
        .has_subcommand = false,
    };
    swbt_switch_subcommand_dispatcher_response_t response;

    int failed = 0;
    failed += expect_eq_int(swbt_switch_subcommand_dispatch(&config, &output, &response),
                            SWBT_SWITCH_SUBCOMMAND_DISPATCH_NO_REPLY);
    failed += expect_eq_action(response.action, SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_NONE);
    failed += expect_eq_size(response.report_size, 0u);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += test_simple_ack_dispatches_set_report_mode_reply();
    failed += test_low_power_mode_dispatches_simple_ack_reply();
    failed += test_trigger_buttons_elapsed_time_builds_pairing_reply();
    failed += test_spi_flash_read_builds_reply_data_from_virtual_spi();
    failed += test_set_player_lights_updates_state_and_builds_ack();
    failed += test_get_player_lights_builds_current_state_reply();
    failed += test_malformed_player_lights_request_does_not_update_state();
    failed += test_request_device_info_builds_pro_controller_identity_reply();
    failed += test_request_device_info_accepts_mizuyoukanao_pro_profile();
    failed += test_malformed_spi_request_does_not_build_reply();
    failed += test_rumble_only_report_has_no_reply_action();
    return failed == 0 ? 0 : 1;
}
