#include "switch/switch_spi_seed.h"

typedef struct {
    uint32_t address;
    size_t data_len;
} seed_range_t;

static const uint8_t dev_device_type[] = {SWBT_SWITCH_SPI_DEVICE_TYPE_PRO_CONTROLLER};

static const uint8_t dev_controller_colors[] = {
    0x0D, 0x0D, 0x0D, 0xFF, 0xFF, 0xFF,
};

static const uint8_t dev_factory_imu_calibration[] = {
    0xB0, 0xFF, 0xB9, 0xFE, 0xE0, 0x00, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
    0x0E, 0x00, 0xDF, 0xFF, 0xD0, 0xFF, 0x3B, 0x34, 0x3B, 0x34, 0x3B, 0x34,
};

static const uint8_t dev_left_stick_calibration[] = {
    0xB2, 0xA1, 0x20, 0x80, 0xF0, 0x7F, 0x10, 0x80, 0x00,
};

static const uint8_t dev_right_stick_calibration[] = {
    0xB2, 0xA1, 0x30, 0x80, 0xE0, 0x7F, 0x20, 0x80, 0x00,
};

static const uint8_t dev_user_magic[] = {
    0xB2,
    0xA1,
};

static swbt_switch_spi_result_t validate_required_field(const uint8_t *data, size_t data_len,
                                                        size_t expected_len) {
    if (data == NULL || data_len != expected_len) {
        return SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT;
    }
    return SWBT_SWITCH_SPI_OK;
}

static swbt_switch_spi_result_t validate_entry_boundary(const swbt_switch_spi_seed_entry_t *entry) {
    uint32_t span = 0;

    if (entry == NULL || entry->data == NULL || entry->data_len == 0) {
        return SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT;
    }
    if (entry->data_len > SWBT_SWITCH_SPI_STORAGE_SIZE) {
        return SWBT_SWITCH_SPI_ERROR_ADDRESS_OUT_OF_RANGE;
    }
    span = (uint32_t)entry->data_len;
    if (entry->address > (SWBT_SWITCH_SPI_STORAGE_SIZE - span)) {
        return SWBT_SWITCH_SPI_ERROR_ADDRESS_OUT_OF_RANGE;
    }
    return SWBT_SWITCH_SPI_OK;
}

static int ranges_overlap(seed_range_t lhs, seed_range_t rhs) {
    const uint32_t lhs_end = lhs.address + (uint32_t)lhs.data_len;
    const uint32_t rhs_end = rhs.address + (uint32_t)rhs.data_len;
    return lhs.address < rhs_end && rhs.address < lhs_end;
}

static swbt_switch_spi_result_t
validate_extra_entries(const swbt_switch_spi_seed_profile_t *profile) {
    static const seed_range_t fixed_ranges[] = {
        {SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE, SWBT_SWITCH_SPI_SEED_DEVICE_TYPE_SIZE},
        {SWBT_SWITCH_SPI_ADDRESS_CONTROLLER_COLORS, SWBT_SWITCH_SPI_SEED_CONTROLLER_COLORS_SIZE},
        {SWBT_SWITCH_SPI_ADDRESS_FACTORY_IMU_CALIBRATION,
         SWBT_SWITCH_SPI_SEED_FACTORY_IMU_CALIBRATION_SIZE},
        {SWBT_SWITCH_SPI_ADDRESS_LEFT_STICK_CALIBRATION,
         SWBT_SWITCH_SPI_SEED_STICK_CALIBRATION_SIZE},
        {SWBT_SWITCH_SPI_ADDRESS_RIGHT_STICK_CALIBRATION,
         SWBT_SWITCH_SPI_SEED_STICK_CALIBRATION_SIZE},
        {SWBT_SWITCH_SPI_ADDRESS_USER_LEFT_STICK_MAGIC, SWBT_SWITCH_SPI_SEED_USER_MAGIC_SIZE},
        {SWBT_SWITCH_SPI_ADDRESS_USER_RIGHT_STICK_MAGIC, SWBT_SWITCH_SPI_SEED_USER_MAGIC_SIZE},
        {SWBT_SWITCH_SPI_ADDRESS_USER_IMU_MAGIC, SWBT_SWITCH_SPI_SEED_USER_MAGIC_SIZE},
    };

    if (profile->extra_entry_count == 0) {
        return SWBT_SWITCH_SPI_OK;
    }
    if (profile->extra_entries == NULL) {
        return SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT;
    }

    for (size_t index = 0; index < profile->extra_entry_count; ++index) {
        const swbt_switch_spi_seed_entry_t *entry = &profile->extra_entries[index];
        const seed_range_t entry_range = {
            .address = entry->address,
            .data_len = entry->data_len,
        };
        swbt_switch_spi_result_t result = validate_entry_boundary(entry);
        if (result != SWBT_SWITCH_SPI_OK) {
            return result;
        }

        for (size_t fixed_index = 0; fixed_index < sizeof(fixed_ranges) / sizeof(fixed_ranges[0]);
             ++fixed_index) {
            if (ranges_overlap(entry_range, fixed_ranges[fixed_index])) {
                return SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT;
            }
        }
        for (size_t previous_index = 0; previous_index < index; ++previous_index) {
            const swbt_switch_spi_seed_entry_t *previous = &profile->extra_entries[previous_index];
            const seed_range_t previous_range = {
                .address = previous->address,
                .data_len = previous->data_len,
            };
            if (ranges_overlap(entry_range, previous_range)) {
                return SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT;
            }
        }
    }
    return SWBT_SWITCH_SPI_OK;
}

static swbt_switch_spi_result_t validate_profile(const swbt_switch_spi_seed_profile_t *profile) {
    swbt_switch_spi_result_t result = SWBT_SWITCH_SPI_OK;

    if (profile == NULL) {
        return SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT;
    }

    result = validate_required_field(profile->device_type, profile->device_type_len,
                                     SWBT_SWITCH_SPI_SEED_DEVICE_TYPE_SIZE);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = validate_required_field(profile->controller_colors, profile->controller_colors_len,
                                     SWBT_SWITCH_SPI_SEED_CONTROLLER_COLORS_SIZE);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = validate_required_field(profile->factory_imu_calibration,
                                     profile->factory_imu_calibration_len,
                                     SWBT_SWITCH_SPI_SEED_FACTORY_IMU_CALIBRATION_SIZE);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = validate_required_field(profile->left_stick_calibration,
                                     profile->left_stick_calibration_len,
                                     SWBT_SWITCH_SPI_SEED_STICK_CALIBRATION_SIZE);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = validate_required_field(profile->right_stick_calibration,
                                     profile->right_stick_calibration_len,
                                     SWBT_SWITCH_SPI_SEED_STICK_CALIBRATION_SIZE);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result =
        validate_required_field(profile->user_left_stick_magic, profile->user_left_stick_magic_len,
                                SWBT_SWITCH_SPI_SEED_USER_MAGIC_SIZE);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = validate_required_field(profile->user_right_stick_magic,
                                     profile->user_right_stick_magic_len,
                                     SWBT_SWITCH_SPI_SEED_USER_MAGIC_SIZE);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = validate_required_field(profile->user_imu_magic, profile->user_imu_magic_len,
                                     SWBT_SWITCH_SPI_SEED_USER_MAGIC_SIZE);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    return validate_extra_entries(profile);
}

static swbt_switch_spi_result_t write_entry(swbt_switch_spi_t *spi, uint32_t address,
                                            const uint8_t *data, size_t data_len) {
    return swbt_switch_spi_write(spi, address, data, data_len);
}

swbt_switch_spi_seed_profile_t swbt_switch_spi_seed_dev_profile(void) {
    const swbt_switch_spi_seed_profile_t profile = {
        .device_type = dev_device_type,
        .device_type_len = sizeof(dev_device_type),
        .controller_colors = dev_controller_colors,
        .controller_colors_len = sizeof(dev_controller_colors),
        .factory_imu_calibration = dev_factory_imu_calibration,
        .factory_imu_calibration_len = sizeof(dev_factory_imu_calibration),
        .left_stick_calibration = dev_left_stick_calibration,
        .left_stick_calibration_len = sizeof(dev_left_stick_calibration),
        .right_stick_calibration = dev_right_stick_calibration,
        .right_stick_calibration_len = sizeof(dev_right_stick_calibration),
        .user_left_stick_magic = dev_user_magic,
        .user_left_stick_magic_len = sizeof(dev_user_magic),
        .user_right_stick_magic = dev_user_magic,
        .user_right_stick_magic_len = sizeof(dev_user_magic),
        .user_imu_magic = dev_user_magic,
        .user_imu_magic_len = sizeof(dev_user_magic),
        .extra_entries = NULL,
        .extra_entry_count = 0,
    };
    return profile;
}

swbt_switch_spi_result_t swbt_switch_spi_seed_apply(swbt_switch_spi_t *spi,
                                                    const swbt_switch_spi_seed_profile_t *profile) {
    swbt_switch_spi_result_t result = SWBT_SWITCH_SPI_OK;

    if (spi == NULL) {
        return SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT;
    }

    result = validate_profile(profile);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }

    result = write_entry(spi, SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE, profile->device_type,
                         profile->device_type_len);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = write_entry(spi, SWBT_SWITCH_SPI_ADDRESS_CONTROLLER_COLORS, profile->controller_colors,
                         profile->controller_colors_len);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = write_entry(spi, SWBT_SWITCH_SPI_ADDRESS_FACTORY_IMU_CALIBRATION,
                         profile->factory_imu_calibration, profile->factory_imu_calibration_len);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = write_entry(spi, SWBT_SWITCH_SPI_ADDRESS_LEFT_STICK_CALIBRATION,
                         profile->left_stick_calibration, profile->left_stick_calibration_len);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = write_entry(spi, SWBT_SWITCH_SPI_ADDRESS_RIGHT_STICK_CALIBRATION,
                         profile->right_stick_calibration, profile->right_stick_calibration_len);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = write_entry(spi, SWBT_SWITCH_SPI_ADDRESS_USER_LEFT_STICK_MAGIC,
                         profile->user_left_stick_magic, profile->user_left_stick_magic_len);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = write_entry(spi, SWBT_SWITCH_SPI_ADDRESS_USER_RIGHT_STICK_MAGIC,
                         profile->user_right_stick_magic, profile->user_right_stick_magic_len);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }
    result = write_entry(spi, SWBT_SWITCH_SPI_ADDRESS_USER_IMU_MAGIC, profile->user_imu_magic,
                         profile->user_imu_magic_len);
    if (result != SWBT_SWITCH_SPI_OK) {
        return result;
    }

    for (size_t index = 0; index < profile->extra_entry_count; ++index) {
        result =
            write_entry(spi, profile->extra_entries[index].address,
                        profile->extra_entries[index].data, profile->extra_entries[index].data_len);
        if (result != SWBT_SWITCH_SPI_OK) {
            return result;
        }
    }
    return SWBT_SWITCH_SPI_OK;
}
