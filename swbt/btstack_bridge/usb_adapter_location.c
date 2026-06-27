#include "btstack_bridge/usb_adapter_location.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static bool swbt_btstack_usb_adapter_location_has_prefix(const char *text, const char *prefix) {
    const size_t prefix_length = strlen(prefix);
    return strncmp(text, prefix, prefix_length) == 0;
}

static bool swbt_btstack_usb_adapter_location_is_digit(char value) {
    return value >= '0' && value <= '9';
}

static bool swbt_btstack_usb_adapter_location_parse_u8(const char **cursor, uint8_t *out_value) {
    int value = 0;
    bool has_digit = false;

    if (cursor == NULL || *cursor == NULL || out_value == NULL) {
        return false;
    }

    while (swbt_btstack_usb_adapter_location_is_digit(**cursor)) {
        has_digit = true;
        value = (value * 10) + (**cursor - '0');
        if (value > 255) {
            return false;
        }
        ++(*cursor);
    }

    if (!has_digit || value == 0) {
        return false;
    }
    *out_value = (uint8_t)value;
    return true;
}

static swbt_btstack_usb_adapter_location_result_t
swbt_btstack_usb_adapter_location_parse_libusb(const char *text,
                                               swbt_btstack_usb_adapter_location_t *out_location) {
    const char *cursor = text + strlen("libusb:");
    uint8_t bus = 0u;
    int port_count = 0;
    uint8_t ports[SWBT_BTSTACK_USB_ADAPTER_LOCATION_MAX_PORTS] = {0};

    if (!swbt_btstack_usb_adapter_location_parse_u8(&cursor, &bus) || *cursor != ':') {
        return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_FORMAT;
    }
    ++cursor;

    if (*cursor == '\0') {
        return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_FORMAT;
    }

    while (*cursor != '\0') {
        if (port_count >= SWBT_BTSTACK_USB_ADAPTER_LOCATION_MAX_PORTS) {
            return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_FORMAT;
        }
        if (!swbt_btstack_usb_adapter_location_parse_u8(&cursor, &ports[port_count])) {
            return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_FORMAT;
        }
        ++port_count;

        if (*cursor == '.') {
            ++cursor;
            if (*cursor == '\0') {
                return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_FORMAT;
            }
            continue;
        }
        if (*cursor != '\0') {
            return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_FORMAT;
        }
    }

    *out_location = (swbt_btstack_usb_adapter_location_t){
        .backend = SWBT_BTSTACK_USB_ADAPTER_LOCATION_BACKEND_LIBUSB,
        .libusb_bus = bus,
        .libusb_port_count = port_count,
    };
    for (int index = 0; index < port_count; ++index) {
        out_location->libusb_ports[index] = ports[index];
    }
    return SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK;
}

swbt_btstack_usb_adapter_location_result_t
swbt_btstack_usb_adapter_location_parse(const char *text,
                                        swbt_btstack_usb_adapter_location_t *out_location) {
    if (text == NULL || out_location == NULL) {
        return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_ARGUMENT;
    }

    *out_location = (swbt_btstack_usb_adapter_location_t){0};

    if (swbt_btstack_usb_adapter_location_has_prefix(text, "winusb:")) {
        const char *location_path = text + strlen("winusb:");
        if (location_path[0] == '\0') {
            return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_FORMAT;
        }
        *out_location = (swbt_btstack_usb_adapter_location_t){
            .backend = SWBT_BTSTACK_USB_ADAPTER_LOCATION_BACKEND_WINUSB,
            .winusb_location_path = location_path,
        };
        return SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK;
    }

    if (swbt_btstack_usb_adapter_location_has_prefix(text, "libusb:")) {
        return swbt_btstack_usb_adapter_location_parse_libusb(text, out_location);
    }

    return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_FORMAT;
}

swbt_btstack_usb_adapter_location_result_t
swbt_btstack_usb_adapter_location_apply(const swbt_btstack_usb_adapter_location_t *location,
                                        const swbt_btstack_usb_adapter_location_port_t *port) {
    if (location == NULL || port == NULL) {
        return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_ARGUMENT;
    }

    switch (location->backend) {
    case SWBT_BTSTACK_USB_ADAPTER_LOCATION_BACKEND_WINUSB:
        if (location->winusb_location_path == NULL || port->set_winusb_location_path == NULL) {
            return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_UNSUPPORTED_BACKEND;
        }
        return port->set_winusb_location_path(port->context, location->winusb_location_path) == 0
                   ? SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK
                   : SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_BACKEND;
    case SWBT_BTSTACK_USB_ADAPTER_LOCATION_BACKEND_LIBUSB: {
        uint8_t ports[SWBT_BTSTACK_USB_ADAPTER_LOCATION_MAX_PORTS] = {0};
        if (location->libusb_port_count <= 0 ||
            location->libusb_port_count > SWBT_BTSTACK_USB_ADAPTER_LOCATION_MAX_PORTS ||
            port->set_libusb_bus_and_path == NULL) {
            return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_UNSUPPORTED_BACKEND;
        }
        for (int index = 0; index < location->libusb_port_count; ++index) {
            ports[index] = location->libusb_ports[index];
        }
        port->set_libusb_bus_and_path(port->context, location->libusb_bus,
                                      location->libusb_port_count, ports);
        return SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK;
    }
    case SWBT_BTSTACK_USB_ADAPTER_LOCATION_BACKEND_NONE:
    default:
        break;
    }
    return SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_ARGUMENT;
}
