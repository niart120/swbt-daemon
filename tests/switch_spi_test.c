#include <stddef.h>
#include <stdint.h>

#include "switch/switch_spi.h"

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_size(size_t actual, size_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_bytes(const uint8_t *actual, const uint8_t *expected, size_t len) {
    for (size_t index = 0; index < len; ++index) {
        if (actual[index] != expected[index]) {
            return 1;
        }
    }
    return 0;
}

static swbt_switch_spi_read_request_t read_request(uint32_t address, uint8_t size) {
    swbt_switch_spi_read_request_t request = {
        .address = address,
        .size = size,
    };
    return request;
}

int main(void) {
    static swbt_switch_spi_t spi;
    uint8_t out[SWBT_SWITCH_SPI_MAX_READ_SIZE];
    size_t written = 0;
    const uint8_t device_type = SWBT_SWITCH_SPI_DEVICE_TYPE_PRO_CONTROLLER;
    const uint8_t colors[] = {
        0x0D, 0x0D, 0x0D, 0xFF, 0xFF, 0xFF,
    };
    const uint8_t factory_calibration[] = {
        0xB0, 0xFF, 0xB9, 0xFE, 0xE0, 0x00, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
        0x0E, 0x00, 0xDF, 0xFF, 0xD0, 0xFF, 0x3B, 0x34, 0x3B, 0x34, 0x3B, 0x34,
    };

    if (swbt_switch_spi_init(&spi) != SWBT_SWITCH_SPI_OK) {
        return 1;
    }
    if (swbt_switch_spi_read(&spi, read_request(SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE, 1), out,
                             sizeof(out), &written) != SWBT_SWITCH_SPI_OK) {
        return 2;
    }
    if (expect_eq_size(written, 1) || expect_eq_u8(out[0], 0xFFU)) {
        return 3;
    }
    if (swbt_switch_spi_write(&spi, SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE, &device_type, 1) !=
        SWBT_SWITCH_SPI_OK) {
        return 4;
    }
    if (swbt_switch_spi_write(&spi, SWBT_SWITCH_SPI_ADDRESS_CONTROLLER_COLORS, colors,
                              sizeof(colors)) != SWBT_SWITCH_SPI_OK) {
        return 5;
    }
    if (swbt_switch_spi_write(&spi, SWBT_SWITCH_SPI_ADDRESS_FACTORY_IMU_CALIBRATION,
                              factory_calibration,
                              sizeof(factory_calibration)) != SWBT_SWITCH_SPI_OK) {
        return 6;
    }
    if (swbt_switch_spi_read(&spi, read_request(SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE, 1), out,
                             sizeof(out), &written) != SWBT_SWITCH_SPI_OK) {
        return 7;
    }
    if (expect_eq_u8(out[0], SWBT_SWITCH_SPI_DEVICE_TYPE_PRO_CONTROLLER)) {
        return 8;
    }
    if (swbt_switch_spi_read(
            &spi, read_request(SWBT_SWITCH_SPI_ADDRESS_CONTROLLER_COLORS, (uint8_t)sizeof(colors)),
            out, sizeof(out), &written) != SWBT_SWITCH_SPI_OK) {
        return 9;
    }
    if (expect_eq_size(written, sizeof(colors)) || expect_bytes(out, colors, sizeof(colors))) {
        return 10;
    }
    if (swbt_switch_spi_read(&spi,
                             read_request(SWBT_SWITCH_SPI_ADDRESS_FACTORY_IMU_CALIBRATION,
                                          (uint8_t)sizeof(factory_calibration)),
                             out, sizeof(out), &written) != SWBT_SWITCH_SPI_OK) {
        return 11;
    }
    if (expect_eq_size(written, sizeof(factory_calibration)) ||
        expect_bytes(out, factory_calibration, sizeof(factory_calibration))) {
        return 12;
    }
    if (swbt_switch_spi_read(&spi, read_request(SWBT_SWITCH_SPI_ADDRESS_LIMIT - 1U, 1), out,
                             sizeof(out), &written) != SWBT_SWITCH_SPI_OK) {
        return 13;
    }
    if (expect_eq_u8(out[0], 0xFFU)) {
        return 14;
    }
    if (swbt_switch_spi_read(&spi, read_request(SWBT_SWITCH_SPI_ADDRESS_LIMIT, 1), out, sizeof(out),
                             &written) != SWBT_SWITCH_SPI_ERROR_ADDRESS_OUT_OF_RANGE) {
        return 15;
    }
    if (swbt_switch_spi_read(&spi, read_request(SWBT_SWITCH_SPI_ADDRESS_LIMIT - 1U, 2), out,
                             sizeof(out), &written) != SWBT_SWITCH_SPI_ERROR_ADDRESS_OUT_OF_RANGE) {
        return 16;
    }
    if (swbt_switch_spi_read(
            &spi,
            read_request(SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE, SWBT_SWITCH_SPI_MAX_READ_SIZE + 1U),
            out, sizeof(out), &written) != SWBT_SWITCH_SPI_ERROR_READ_TOO_LARGE) {
        return 17;
    }
    if (swbt_switch_spi_read(&spi, read_request(SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE, 2), out, 1,
                             &written) != SWBT_SWITCH_SPI_ERROR_BUFFER_TOO_SMALL) {
        return 18;
    }
    if (expect_eq_size(written, 2)) {
        return 19;
    }
    if (swbt_switch_spi_read(NULL, read_request(SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE, 1), out,
                             sizeof(out), &written) != SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT) {
        return 20;
    }
    if (swbt_switch_spi_write(&spi, SWBT_SWITCH_SPI_STORAGE_SIZE - 1U, colors, 2) !=
        SWBT_SWITCH_SPI_ERROR_ADDRESS_OUT_OF_RANGE) {
        return 21;
    }

    return 0;
}
