#ifndef SWBT_BTSTACK_BRIDGE_USB_ADAPTER_LOCATION_H
#define SWBT_BTSTACK_BRIDGE_USB_ADAPTER_LOCATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SWBT_BTSTACK_USB_ADAPTER_LOCATION_MAX_PORTS 7

typedef enum {
    SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK = 0,
    SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_ARGUMENT = -1,
    SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_FORMAT = -2,
    SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_UNSUPPORTED_BACKEND = -3,
    SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_BACKEND = -4,
} swbt_btstack_usb_adapter_location_result_t;

typedef enum {
    SWBT_BTSTACK_USB_ADAPTER_LOCATION_BACKEND_NONE = 0,
    SWBT_BTSTACK_USB_ADAPTER_LOCATION_BACKEND_WINUSB = 1,
    SWBT_BTSTACK_USB_ADAPTER_LOCATION_BACKEND_LIBUSB = 2,
} swbt_btstack_usb_adapter_location_backend_t;

typedef struct {
    swbt_btstack_usb_adapter_location_backend_t backend;
    const char *winusb_location_path;
    uint8_t libusb_bus;
    int libusb_port_count;
    uint8_t libusb_ports[SWBT_BTSTACK_USB_ADAPTER_LOCATION_MAX_PORTS];
} swbt_btstack_usb_adapter_location_t;

typedef struct {
    int (*set_winusb_location_path)(void *context, const char *location_path);
    void (*set_libusb_bus_and_path)(void *context, uint8_t bus, int port_count, uint8_t *ports);
    void *context;
} swbt_btstack_usb_adapter_location_port_t;

swbt_btstack_usb_adapter_location_result_t
swbt_btstack_usb_adapter_location_parse(const char *text,
                                        swbt_btstack_usb_adapter_location_t *out_location);

swbt_btstack_usb_adapter_location_result_t
swbt_btstack_usb_adapter_location_apply(const swbt_btstack_usb_adapter_location_t *location,
                                        const swbt_btstack_usb_adapter_location_port_t *port);

#ifdef __cplusplus
}
#endif

#endif
