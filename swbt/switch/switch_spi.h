#ifndef SWBT_SWITCH_SPI_H
#define SWBT_SWITCH_SPI_H

#include <stddef.h>
#include <stdint.h>

#define SWBT_SWITCH_SPI_ADDRESS_LIMIT 0x80000u
#define SWBT_SWITCH_SPI_STORAGE_SIZE 0x10000u
#define SWBT_SWITCH_SPI_MAX_READ_SIZE 0x1Du
#define SWBT_SWITCH_SPI_ERASED_BYTE 0xFFu

#define SWBT_SWITCH_SPI_ADDRESS_SERIAL_NUMBER 0x6000u
#define SWBT_SWITCH_SPI_ADDRESS_DEVICE_TYPE 0x6012u
#define SWBT_SWITCH_SPI_ADDRESS_COLOR_INFO_EXISTS 0x601Bu
#define SWBT_SWITCH_SPI_ADDRESS_FACTORY_IMU_CALIBRATION 0x6020u
#define SWBT_SWITCH_SPI_ADDRESS_LEFT_STICK_CALIBRATION 0x603Du
#define SWBT_SWITCH_SPI_ADDRESS_RIGHT_STICK_CALIBRATION 0x6046u
#define SWBT_SWITCH_SPI_ADDRESS_CONTROLLER_COLORS 0x6050u
#define SWBT_SWITCH_SPI_ADDRESS_USER_LEFT_STICK_MAGIC 0x8010u
#define SWBT_SWITCH_SPI_ADDRESS_USER_RIGHT_STICK_MAGIC 0x801Bu
#define SWBT_SWITCH_SPI_ADDRESS_USER_IMU_MAGIC 0x8026u

#define SWBT_SWITCH_SPI_DEVICE_TYPE_PRO_CONTROLLER 0x03u

typedef enum {
    SWBT_SWITCH_SPI_OK = 0,
    SWBT_SWITCH_SPI_ERROR_INVALID_ARGUMENT = -1,
    SWBT_SWITCH_SPI_ERROR_ADDRESS_OUT_OF_RANGE = -2,
    SWBT_SWITCH_SPI_ERROR_READ_TOO_LARGE = -3,
    SWBT_SWITCH_SPI_ERROR_BUFFER_TOO_SMALL = -4,
} swbt_switch_spi_result_t;

typedef struct {
    uint8_t data[SWBT_SWITCH_SPI_STORAGE_SIZE];
    uint8_t default_value;
} swbt_switch_spi_t;

typedef struct {
    uint32_t address;
    uint8_t size;
} swbt_switch_spi_read_request_t;

swbt_switch_spi_result_t swbt_switch_spi_init(swbt_switch_spi_t *spi);

swbt_switch_spi_result_t swbt_switch_spi_write(swbt_switch_spi_t *spi, uint32_t address,
                                               const uint8_t *data, size_t data_len);

swbt_switch_spi_result_t swbt_switch_spi_read(const swbt_switch_spi_t *spi,
                                              swbt_switch_spi_read_request_t request, uint8_t *out,
                                              size_t out_capacity, size_t *out_written);

#endif
