#include "switch/switch_subcommand_dispatcher.h"

#define SWBT_SWITCH_SPI_READ_REQUEST_SIZE 5u
#define SWBT_SWITCH_SPI_READ_REPLY_PREFIX_SIZE 5u
#define SWBT_SWITCH_TRIGGER_BUTTONS_ELAPSED_REPLY_DATA_SIZE 14u
#define SWBT_SWITCH_TRIGGER_BUTTONS_ELAPSED_PAIRING_HELD_TICKS 300u
#define SWBT_SWITCH_MCU_CONFIG_REPLY_DATA_SIZE 34u

static void swbt_switch_subcommand_dispatcher_reset_response(
    swbt_switch_subcommand_dispatcher_response_t *response) {
    response->action = SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_NONE;
    response->report_size = 0;
    for (size_t index = 0; index < SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE; ++index) {
        response->report[index] = 0;
    }
}

static swbt_switch_subcommand_dispatch_result_t swbt_switch_subcommand_dispatcher_build_reply(
    const swbt_switch_subcommand_dispatcher_config_t *config,
    swbt_switch_subcommand_dispatcher_response_t *response, uint8_t ack, uint8_t subcommand_id,
    const uint8_t *data, size_t data_len) {
    size_t written = 0;
    const swbt_switch_subcommand_reply_t reply = {
        .ack = ack,
        .subcommand_id = subcommand_id,
        .data = data,
        .data_len = data_len,
    };

    const swbt_switch_subcommand_reply_result_t result =
        swbt_switch_build_subcommand_reply(config->state, config->report_options, &reply,
                                           response->report, sizeof(response->report), &written);
    if (result != SWBT_SWITCH_SUBCOMMAND_REPLY_OK) {
        swbt_switch_subcommand_dispatcher_reset_response(response);
        return SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_REPLY_BUILD_FAILED;
    }

    response->action = SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY;
    response->report_size = written;
    return SWBT_SWITCH_SUBCOMMAND_DISPATCH_OK;
}

static swbt_switch_subcommand_dispatch_result_t swbt_switch_subcommand_dispatcher_simple_ack(
    const swbt_switch_subcommand_dispatcher_config_t *config,
    const swbt_switch_output_report_t *output_report,
    swbt_switch_subcommand_dispatcher_response_t *response) {
    return swbt_switch_subcommand_dispatcher_build_reply(config, response,
                                                         SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SIMPLE,
                                                         output_report->subcommand_id, NULL, 0);
}

static swbt_switch_subcommand_dispatch_result_t
swbt_switch_subcommand_dispatcher_request_device_info(
    const swbt_switch_subcommand_dispatcher_config_t *config,
    const swbt_switch_output_report_t *output_report,
    swbt_switch_subcommand_dispatcher_response_t *response) {
    uint8_t reply_data[SWBT_SWITCH_DEVICE_INFO_REPLY_DATA_SIZE];

    if (config->device_info == NULL) {
        return SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_INVALID_ARGUMENT;
    }

    swbt_switch_device_info_write_reply_data(config->device_info, reply_data);
    return swbt_switch_subcommand_dispatcher_build_reply(
        config, response, SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_DEVICE_INFO,
        output_report->subcommand_id, reply_data, sizeof(reply_data));
}

static void swbt_switch_subcommand_dispatcher_write_u16_le(uint8_t *data, size_t offset,
                                                           uint16_t value) {
    data[offset] = (uint8_t)(value & 0xFFu);
    data[offset + 1u] = (uint8_t)((value >> 8u) & 0xFFu);
}

static swbt_switch_subcommand_dispatch_result_t
swbt_switch_subcommand_dispatcher_trigger_buttons_elapsed(
    const swbt_switch_subcommand_dispatcher_config_t *config,
    const swbt_switch_output_report_t *output_report,
    swbt_switch_subcommand_dispatcher_response_t *response) {
    uint8_t reply_data[SWBT_SWITCH_TRIGGER_BUTTONS_ELAPSED_REPLY_DATA_SIZE] = {0};

    swbt_switch_subcommand_dispatcher_write_u16_le(
        reply_data, 0u, SWBT_SWITCH_TRIGGER_BUTTONS_ELAPSED_PAIRING_HELD_TICKS);
    swbt_switch_subcommand_dispatcher_write_u16_le(
        reply_data, 2u, SWBT_SWITCH_TRIGGER_BUTTONS_ELAPSED_PAIRING_HELD_TICKS);

    return swbt_switch_subcommand_dispatcher_build_reply(
        config, response, SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_TRIGGER_BUTTONS_ELAPSED,
        output_report->subcommand_id, reply_data, sizeof(reply_data));
}

static swbt_switch_subcommand_dispatch_result_t swbt_switch_subcommand_dispatcher_set_mcu_config(
    const swbt_switch_subcommand_dispatcher_config_t *config,
    const swbt_switch_output_report_t *output_report,
    swbt_switch_subcommand_dispatcher_response_t *response) {
    static const uint8_t reply_data[SWBT_SWITCH_MCU_CONFIG_REPLY_DATA_SIZE] = {
        0x01u, 0x00u, 0xFFu, 0x00u, 0x08u, 0x00u, 0x1Bu, 0x01u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xC8u,
    };

    return swbt_switch_subcommand_dispatcher_build_reply(
        config, response, SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_MCU_CONFIG,
        output_report->subcommand_id, reply_data, sizeof(reply_data));
}

static uint32_t swbt_switch_subcommand_dispatcher_read_u32_le(const uint8_t *data) {
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8u) | ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

static swbt_switch_subcommand_dispatch_result_t
swbt_switch_subcommand_dispatcher_spi_read(const swbt_switch_subcommand_dispatcher_config_t *config,
                                           const swbt_switch_output_report_t *output_report,
                                           swbt_switch_subcommand_dispatcher_response_t *response) {
    uint8_t reply_data[SWBT_SWITCH_SPI_READ_REPLY_PREFIX_SIZE + SWBT_SWITCH_SPI_MAX_READ_SIZE];
    size_t read_size = 0;

    if (config->spi == NULL) {
        return SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_INVALID_ARGUMENT;
    }
    if (output_report->subcommand_data == NULL ||
        output_report->subcommand_data_len < SWBT_SWITCH_SPI_READ_REQUEST_SIZE) {
        return SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_MALFORMED_SUBCOMMAND;
    }

    const uint32_t address =
        swbt_switch_subcommand_dispatcher_read_u32_le(output_report->subcommand_data);
    const uint8_t size = output_report->subcommand_data[4];
    const swbt_switch_spi_read_request_t request = {
        .address = address,
        .size = size,
    };

    for (size_t index = 0; index < SWBT_SWITCH_SPI_READ_REQUEST_SIZE; ++index) {
        reply_data[index] = output_report->subcommand_data[index];
    }

    const swbt_switch_spi_result_t spi_result = swbt_switch_spi_read(
        config->spi, request, &reply_data[SWBT_SWITCH_SPI_READ_REPLY_PREFIX_SIZE],
        SWBT_SWITCH_SPI_MAX_READ_SIZE, &read_size);
    if (spi_result != SWBT_SWITCH_SPI_OK) {
        return SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_SPI_READ_FAILED;
    }

    return swbt_switch_subcommand_dispatcher_build_reply(
        config, response, SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SPI_READ, output_report->subcommand_id,
        reply_data, SWBT_SWITCH_SPI_READ_REPLY_PREFIX_SIZE + read_size);
}

static swbt_switch_subcommand_dispatch_result_t swbt_switch_subcommand_dispatcher_set_player_lights(
    const swbt_switch_subcommand_dispatcher_config_t *config,
    const swbt_switch_output_report_t *output_report,
    swbt_switch_subcommand_dispatcher_response_t *response) {
    if (config->player_lights == NULL) {
        return SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_INVALID_ARGUMENT;
    }

    const swbt_switch_player_lights_result_t player_lights_result =
        swbt_switch_player_lights_apply_set(config->player_lights, output_report->subcommand_data,
                                            output_report->subcommand_data_len);
    if (player_lights_result != SWBT_SWITCH_PLAYER_LIGHTS_OK) {
        return SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_MALFORMED_SUBCOMMAND;
    }

    return swbt_switch_subcommand_dispatcher_simple_ack(config, output_report, response);
}

static swbt_switch_subcommand_dispatch_result_t swbt_switch_subcommand_dispatcher_get_player_lights(
    const swbt_switch_subcommand_dispatcher_config_t *config,
    const swbt_switch_output_report_t *output_report,
    swbt_switch_subcommand_dispatcher_response_t *response) {
    uint8_t reply_data[SWBT_SWITCH_PLAYER_LIGHTS_REPLY_DATA_SIZE];
    size_t reply_data_len = 0;

    if (config->player_lights == NULL) {
        return SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_INVALID_ARGUMENT;
    }

    const swbt_switch_player_lights_result_t player_lights_result =
        swbt_switch_player_lights_get_reply_data(config->player_lights, reply_data,
                                                 sizeof(reply_data), &reply_data_len);
    if (player_lights_result != SWBT_SWITCH_PLAYER_LIGHTS_OK) {
        return SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_PLAYER_LIGHTS_FAILED;
    }

    return swbt_switch_subcommand_dispatcher_build_reply(
        config, response, SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_PLAYER_LIGHTS,
        output_report->subcommand_id, reply_data, reply_data_len);
}

swbt_switch_subcommand_dispatch_result_t
swbt_switch_subcommand_dispatch(const swbt_switch_subcommand_dispatcher_config_t *config,
                                const swbt_switch_output_report_t *output_report,
                                swbt_switch_subcommand_dispatcher_response_t *response) {
    if (response != NULL) {
        swbt_switch_subcommand_dispatcher_reset_response(response);
    }
    if (config == NULL || output_report == NULL || response == NULL || config->state == NULL ||
        config->report_options == NULL) {
        return SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_INVALID_ARGUMENT;
    }
    if (!output_report->has_subcommand) {
        return SWBT_SWITCH_SUBCOMMAND_DISPATCH_NO_REPLY;
    }

    switch (output_report->subcommand_id) {
    case SWBT_SWITCH_SUBCOMMAND_REQUEST_DEVICE_INFO:
        return swbt_switch_subcommand_dispatcher_request_device_info(config, output_report,
                                                                     response);
    case SWBT_SWITCH_SUBCOMMAND_TRIGGER_BUTTONS_ELAPSED:
        return swbt_switch_subcommand_dispatcher_trigger_buttons_elapsed(config, output_report,
                                                                         response);
    case SWBT_SWITCH_SUBCOMMAND_SET_MCU_CONFIG:
        return swbt_switch_subcommand_dispatcher_set_mcu_config(config, output_report, response);
    case SWBT_SWITCH_SUBCOMMAND_LOW_POWER_MODE:
    case SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE:
    case SWBT_SWITCH_SUBCOMMAND_ENABLE_IMU:
    case SWBT_SWITCH_SUBCOMMAND_ENABLE_VIBRATION:
        return swbt_switch_subcommand_dispatcher_simple_ack(config, output_report, response);
    case SWBT_SWITCH_SUBCOMMAND_SPI_FLASH_READ:
        return swbt_switch_subcommand_dispatcher_spi_read(config, output_report, response);
    case SWBT_SWITCH_SUBCOMMAND_SET_PLAYER_LIGHTS:
        return swbt_switch_subcommand_dispatcher_set_player_lights(config, output_report, response);
    case SWBT_SWITCH_SUBCOMMAND_GET_PLAYER_LIGHTS:
        return swbt_switch_subcommand_dispatcher_get_player_lights(config, output_report, response);
    default:
        return SWBT_SWITCH_SUBCOMMAND_DISPATCH_UNSUPPORTED;
    }
}
