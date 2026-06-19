#include <stddef.h>
#include <stdint.h>

#include "switch/switch_spi.h"
#include "switch/switch_spi_seed.h"

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_result(swbt_switch_spi_result_t actual, swbt_switch_spi_result_t expected) {
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

static int read_bytes(const swbt_switch_spi_t *spi, uint32_t address, uint8_t size, uint8_t *out) {
    size_t written = 0;
    if (swbt_switch_spi_read(spi, read_request(address, size), out, size, &written) !=
        SWBT_SWITCH_SPI_OK) {
        return 1;
    }
    return written == size ? 0 : 1;
}

static int expect_range(const swbt_switch_spi_t *spi, uint32_t address, const uint8_t *expected,
                        uint8_t size) {
    uint8_t out[SWBT_SWITCH_SPI_MAX_READ_SIZE] = {0};
    if (read_bytes(spi, address, size, out) != 0) {
        return 1;
    }
    return expect_bytes(out, expected, size);
}

static int apply_dev_profile_writes_expected_ranges(void) {
    static swbt_switch_spi_t spi;
    const swbt_switch_spi_seed_profile_t profile = swbt_switch_spi_seed_dev_profile();
    const uint8_t expected_device_type[] = {SWBT_SWITCH_SPI_DEVICE_TYPE_PRO_CONTROLLER};
    const uint8_t expected_colors[] = {
        0x0D, 0x0D, 0x0D, 0xFF, 0xFF, 0xFF,
    };
    const uint8_t expected_factory_imu[] = {
        0xB0, 0xFF, 0xB9, 0xFE, 0xE0, 0x00, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
        0x0E, 0x00, 0xDF, 0xFF, 0xD0, 0xFF, 0x3B, 0x34, 0x3B, 0x34, 0x3B, 0x34,
    };
    const uint8_t expected_left_stick[] = {
        0xB2, 0xA1, 0x20, 0x80, 0xF0, 0x7F, 0x10, 0x80, 0x00,
    };
    const uint8_t expected_right_stick[] = {
        0xB2, 0xA1, 0x30, 0x80, 0xE0, 0x7F, 0x20, 0x80, 0x00,
    };
    const uint8_t expected_magic[] = {0xB2, 0xA1};

    if (swbt_switch_spi_init(&spi) != SWBT_SWITCH_SPI_OK) {
        return 1;
    }
    if (swbt_switch_spi_seed_apply(&spi, &profile) != SWBT_SWITCH_SPI_OK) {
        return 2;
    }
    if (expect_range(&spi, SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE, expected_device_type,
                     (uint8_t)sizeof(expected_device_type))) {
        return 3;
    }
    if (expect_range(&spi, SWBT_SWITCH_SPI_ADDRESS_CONTROLLER_COLORS, expected_colors,
                     (uint8_t)sizeof(expected_colors))) {
        return 4;
    }
    if (expect_range(&spi, SWBT_SWITCH_SPI_ADDRESS_FACTORY_IMU_CALIBRATION, expected_factory_imu,
                     (uint8_t)sizeof(expected_factory_imu))) {
        return 5;
    }
    if (expect_range(&spi, SWBT_SWITCH_SPI_ADDRESS_LEFT_STICK_CALIBRATION, expected_left_stick,
                     (uint8_t)sizeof(expected_left_stick))) {
        return 6;
    }
    if (expect_range(&spi, SWBT_SWITCH_SPI_ADDRESS_RIGHT_STICK_CALIBRATION, expected_right_stick,
                     (uint8_t)sizeof(expected_right_stick))) {
        return 7;
    }
    if (expect_range(&spi, SWBT_SWITCH_SPI_ADDRESS_USER_LEFT_STICK_MAGIC, expected_magic,
                     (uint8_t)sizeof(expected_magic))) {
        return 8;
    }
    if (expect_range(&spi, SWBT_SWITCH_SPI_ADDRESS_USER_RIGHT_STICK_MAGIC, expected_magic,
                     (uint8_t)sizeof(expected_magic))) {
        return 9;
    }
    if (expect_range(&spi, SWBT_SWITCH_SPI_ADDRESS_USER_IMU_MAGIC, expected_magic,
                     (uint8_t)sizeof(expected_magic))) {
        return 10;
    }
    return 0;
}

static int wrong_lengths_are_rejected(void) {
    static swbt_switch_spi_t spi;
    swbt_switch_spi_seed_profile_t profile = swbt_switch_spi_seed_dev_profile();

    if (swbt_switch_spi_init(&spi) != SWBT_SWITCH_SPI_OK) {
        return 1;
    }

    profile.controller_colors_len = 5;
    if (expect_eq_result(swbt_switch_spi_seed_apply(&spi, &profile),
                         SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT)) {
        return 2;
    }

    profile = swbt_switch_spi_seed_dev_profile();
    profile.factory_imu_calibration_len = 23;
    if (expect_eq_result(swbt_switch_spi_seed_apply(&spi, &profile),
                         SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT)) {
        return 3;
    }

    profile = swbt_switch_spi_seed_dev_profile();
    profile.left_stick_calibration_len = 8;
    if (expect_eq_result(swbt_switch_spi_seed_apply(&spi, &profile),
                         SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT)) {
        return 4;
    }

    profile = swbt_switch_spi_seed_dev_profile();
    profile.right_stick_calibration_len = 8;
    if (expect_eq_result(swbt_switch_spi_seed_apply(&spi, &profile),
                         SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT)) {
        return 5;
    }

    profile = swbt_switch_spi_seed_dev_profile();
    profile.user_imu_magic_len = 1;
    if (expect_eq_result(swbt_switch_spi_seed_apply(&spi, &profile),
                         SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT)) {
        return 6;
    }

    return 0;
}

static int unseeded_ranges_remain_erased(void) {
    static swbt_switch_spi_t spi;
    uint8_t out = 0;
    const swbt_switch_spi_seed_profile_t profile = swbt_switch_spi_seed_dev_profile();

    if (swbt_switch_spi_init(&spi) != SWBT_SWITCH_SPI_OK) {
        return 1;
    }
    if (swbt_switch_spi_seed_apply(&spi, &profile) != SWBT_SWITCH_SPI_OK) {
        return 2;
    }
    if (read_bytes(&spi, SWBT_SWITCH_SPI_ADDRESS_SERIAL_NUMBER, 1, &out) != 0) {
        return 3;
    }
    if (expect_eq_u8(out, SWBT_SWITCH_SPI_ERASED_BYTE)) {
        return 4;
    }
    if (read_bytes(&spi, 0x6060u, 1, &out) != 0) {
        return 5;
    }
    if (expect_eq_u8(out, SWBT_SWITCH_SPI_ERASED_BYTE)) {
        return 6;
    }
    return 0;
}

static int out_of_range_entry_is_rejected_before_writes(void) {
    static swbt_switch_spi_t spi;
    uint8_t out = 0;
    const uint8_t extra_value[] = {0xAB};
    const uint8_t invalid_value[] = {0xCD, 0xEF};
    const swbt_switch_spi_seed_entry_t extra_entries[] = {
        {
            .address = 0x6070u,
            .data = extra_value,
            .data_len = sizeof(extra_value),
        },
        {
            .address = SWBT_SWITCH_SPI_STORAGE_SIZE - 1u,
            .data = invalid_value,
            .data_len = sizeof(invalid_value),
        },
    };
    swbt_switch_spi_seed_profile_t profile = swbt_switch_spi_seed_dev_profile();
    profile.extra_entries = extra_entries;
    profile.extra_entry_count = sizeof(extra_entries) / sizeof(extra_entries[0]);

    if (swbt_switch_spi_init(&spi) != SWBT_SWITCH_SPI_OK) {
        return 1;
    }
    if (expect_eq_result(swbt_switch_spi_seed_apply(&spi, &profile),
                         SWBT_SWITCH_SPI_ERROR_ADDRESS_OUT_OF_RANGE)) {
        return 2;
    }
    if (read_bytes(&spi, SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE, 1, &out) != 0) {
        return 3;
    }
    if (expect_eq_u8(out, SWBT_SWITCH_SPI_ERASED_BYTE)) {
        return 4;
    }
    if (read_bytes(&spi, 0x6070u, 1, &out) != 0) {
        return 5;
    }
    if (expect_eq_u8(out, SWBT_SWITCH_SPI_ERASED_BYTE)) {
        return 6;
    }
    return 0;
}

static int overlapping_extra_entry_is_rejected_before_writes(void) {
    static swbt_switch_spi_t spi;
    uint8_t out = 0;
    const uint8_t duplicate_value[] = {0xAA};
    const swbt_switch_spi_seed_entry_t extra_entries[] = {
        {
            .address = SWBT_SWITCH_SPI_ADDRESS_CONTROLLER_COLORS + 1u,
            .data = duplicate_value,
            .data_len = sizeof(duplicate_value),
        },
    };
    swbt_switch_spi_seed_profile_t profile = swbt_switch_spi_seed_dev_profile();
    profile.extra_entries = extra_entries;
    profile.extra_entry_count = sizeof(extra_entries) / sizeof(extra_entries[0]);

    if (swbt_switch_spi_init(&spi) != SWBT_SWITCH_SPI_OK) {
        return 1;
    }
    if (expect_eq_result(swbt_switch_spi_seed_apply(&spi, &profile),
                         SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT)) {
        return 2;
    }
    if (read_bytes(&spi, SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE, 1, &out) != 0) {
        return 3;
    }
    if (expect_eq_u8(out, SWBT_SWITCH_SPI_ERASED_BYTE)) {
        return 4;
    }
    return 0;
}

int main(void) {
    if (apply_dev_profile_writes_expected_ranges() != 0) {
        return 1;
    }
    if (wrong_lengths_are_rejected() != 0) {
        return 2;
    }
    if (unseeded_ranges_remain_erased() != 0) {
        return 3;
    }
    if (out_of_range_entry_is_rejected_before_writes() != 0) {
        return 4;
    }
    if (overlapping_extra_entry_is_rejected_before_writes() != 0) {
        return 5;
    }
    return 0;
}
