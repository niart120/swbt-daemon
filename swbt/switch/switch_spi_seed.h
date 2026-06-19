#ifndef SWBT_SWITCH_SPI_SEED_H
#define SWBT_SWITCH_SPI_SEED_H

#include <stddef.h>
#include <stdint.h>

#include "switch/switch_spi.h"

#define SWBT_SWITCH_SPI_SEED_DEVICE_TYPE_SIZE 1u
#define SWBT_SWITCH_SPI_SEED_CONTROLLER_COLORS_SIZE 6u
#define SWBT_SWITCH_SPI_SEED_FACTORY_IMU_CALIBRATION_SIZE 24u
#define SWBT_SWITCH_SPI_SEED_STICK_CALIBRATION_SIZE 9u
#define SWBT_SWITCH_SPI_SEED_USER_MAGIC_SIZE 2u

typedef struct {
    uint32_t address;
    const uint8_t *data;
    size_t data_len;
} swbt_switch_spi_seed_entry_t;

typedef struct {
    const uint8_t *device_type;
    size_t device_type_len;
    const uint8_t *controller_colors;
    size_t controller_colors_len;
    const uint8_t *factory_imu_calibration;
    size_t factory_imu_calibration_len;
    const uint8_t *left_stick_calibration;
    size_t left_stick_calibration_len;
    const uint8_t *right_stick_calibration;
    size_t right_stick_calibration_len;
    const uint8_t *user_left_stick_magic;
    size_t user_left_stick_magic_len;
    const uint8_t *user_right_stick_magic;
    size_t user_right_stick_magic_len;
    const uint8_t *user_imu_magic;
    size_t user_imu_magic_len;
    const swbt_switch_spi_seed_entry_t *extra_entries;
    size_t extra_entry_count;
} swbt_switch_spi_seed_profile_t;

swbt_switch_spi_seed_profile_t swbt_switch_spi_seed_dev_profile(void);

swbt_switch_spi_result_t swbt_switch_spi_seed_apply(swbt_switch_spi_t *spi,
                                                    const swbt_switch_spi_seed_profile_t *profile);

#endif
