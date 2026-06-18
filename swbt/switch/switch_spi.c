#include "switch/switch_spi.h"

swbt_switch_spi_result_t swbt_switch_spi_init(swbt_switch_spi_t *spi) {
    if (spi == NULL) {
        return SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT;
    }

    spi->default_value = SWBT_SWITCH_SPI_ERASED_BYTE;
    for (size_t index = 0; index < SWBT_SWITCH_SPI_STORAGE_SIZE; ++index) {
        spi->data[index] = spi->default_value;
    }
    return SWBT_SWITCH_SPI_OK;
}

swbt_switch_spi_result_t swbt_switch_spi_write(swbt_switch_spi_t *spi, uint32_t address,
                                               const uint8_t *data, size_t data_len) {
    uint32_t span = 0;

    if (spi == NULL || data == NULL) {
        return SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT;
    }
    if (data_len > SWBT_SWITCH_SPI_STORAGE_SIZE) {
        return SWBT_SWITCH_SPI_ERROR_ADDRESS_OUT_OF_RANGE;
    }
    span = (uint32_t)data_len;
    if (address > (SWBT_SWITCH_SPI_STORAGE_SIZE - span)) {
        return SWBT_SWITCH_SPI_ERROR_ADDRESS_OUT_OF_RANGE;
    }

    for (size_t index = 0; index < data_len; ++index) {
        spi->data[(size_t)address + index] = data[index];
    }
    return SWBT_SWITCH_SPI_OK;
}

swbt_switch_spi_result_t swbt_switch_spi_read(const swbt_switch_spi_t *spi,
                                              swbt_switch_spi_read_request_t request, uint8_t *out,
                                              size_t out_capacity, size_t *out_written) {
    const uint32_t span = request.size;

    if (out_written != NULL) {
        *out_written = 0;
    }
    if (spi == NULL || out == NULL) {
        return SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT;
    }
    if (request.size > SWBT_SWITCH_SPI_MAX_READ_SIZE) {
        return SWBT_SWITCH_SPI_ERROR_READ_TOO_LARGE;
    }
    if (request.address > (SWBT_SWITCH_SPI_ADDRESS_LIMIT - span)) {
        return SWBT_SWITCH_SPI_ERROR_ADDRESS_OUT_OF_RANGE;
    }
    if (out_capacity < request.size) {
        if (out_written != NULL) {
            *out_written = request.size;
        }
        return SWBT_SWITCH_SPI_ERROR_BUFFER_TOO_SMALL;
    }

    for (size_t index = 0; index < request.size; ++index) {
        const uint32_t absolute_address = request.address + (uint32_t)index;
        out[index] = absolute_address < SWBT_SWITCH_SPI_STORAGE_SIZE ? spi->data[absolute_address]
                                                                     : spi->default_value;
    }
    if (out_written != NULL) {
        *out_written = request.size;
    }
    return SWBT_SWITCH_SPI_OK;
}
