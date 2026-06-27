#include "btstack_bridge/device.h"

#include <limits.h>

static bool swbt_btstack_device_port_is_valid(const swbt_btstack_device_port_t *port) {
    return port != NULL && port->platform_start != NULL && port->platform_stop != NULL &&
           port->hid_register != NULL && port->hid_stop != NULL && port->connect != NULL &&
           port->send != NULL;
}

swbt_btstack_device_result_t swbt_btstack_device_init(swbt_btstack_device_t *device,
                                                      const swbt_btstack_device_port_t *port,
                                                      void *port_context) {
    if (device == NULL || !swbt_btstack_device_port_is_valid(port)) {
        return SWBT_BTSTACK_DEVICE_ERROR_INVALID_ARGUMENT;
    }

    *device = (swbt_btstack_device_t){
        .port = port,
        .port_context = port_context,
        .initialized = true,
    };
    return SWBT_BTSTACK_DEVICE_OK;
}

swbt_btstack_device_result_t swbt_btstack_device_open(swbt_btstack_device_t *device,
                                                      swbt_btstack_device_open_options_t options) {
    if (device == NULL || !device->initialized || options.service_buffer == NULL ||
        options.service_buffer_size == 0u || options.registration == NULL) {
        return SWBT_BTSTACK_DEVICE_ERROR_INVALID_ARGUMENT;
    }
    if (device->hid_registered) {
        return SWBT_BTSTACK_DEVICE_OK;
    }

    if (!device->platform_started) {
        if (device->port->platform_start(device->port_context) != 0) {
            return SWBT_BTSTACK_DEVICE_ERROR_OPEN_FAILED;
        }
        device->platform_started = true;
    }

    if (device->port->hid_register(device->port_context, options.service_buffer,
                                   options.service_buffer_size, options.registration) != 0) {
        if (device->platform_started) {
            device->port->platform_stop(device->port_context);
            device->platform_started = false;
        }
        return SWBT_BTSTACK_DEVICE_ERROR_OPEN_FAILED;
    }

    device->hid_registered = true;
    return SWBT_BTSTACK_DEVICE_OK;
}

swbt_btstack_device_result_t
swbt_btstack_device_connect(swbt_btstack_device_t *device,
                            const swbt_btstack_device_connect_request_t *request,
                            uint16_t *out_hid_cid) {
    if (device == NULL || !device->initialized || request == NULL || out_hid_cid == NULL) {
        return SWBT_BTSTACK_DEVICE_ERROR_INVALID_ARGUMENT;
    }
    if (!device->hid_registered) {
        return SWBT_BTSTACK_DEVICE_ERROR_CLOSED;
    }

    return device->port->connect(device->port_context, request, out_hid_cid) == 0
               ? SWBT_BTSTACK_DEVICE_OK
               : SWBT_BTSTACK_DEVICE_ERROR_CONNECT_FAILED;
}

swbt_btstack_device_result_t swbt_btstack_device_send(swbt_btstack_device_t *device,
                                                      uint16_t hid_cid, const uint8_t *message,
                                                      size_t message_size) {
    if (device == NULL || !device->initialized || message == NULL || message_size == 0u ||
        message_size > UINT16_MAX) {
        return SWBT_BTSTACK_DEVICE_ERROR_INVALID_ARGUMENT;
    }
    if (!device->hid_registered) {
        return SWBT_BTSTACK_DEVICE_ERROR_CLOSED;
    }

    return device->port->send(device->port_context, hid_cid, message, message_size) == 0
               ? SWBT_BTSTACK_DEVICE_OK
               : SWBT_BTSTACK_DEVICE_ERROR_SEND_FAILED;
}

swbt_btstack_device_result_t swbt_btstack_device_recv(swbt_btstack_device_t *device,
                                                      uint8_t packet_type, const uint8_t *packet,
                                                      size_t packet_size,
                                                      swbt_btstack_hid_event_t *out_event) {
    if (device == NULL || !device->initialized || out_event == NULL) {
        return SWBT_BTSTACK_DEVICE_ERROR_INVALID_ARGUMENT;
    }
    if (!device->hid_registered) {
        return SWBT_BTSTACK_DEVICE_ERROR_CLOSED;
    }

    const swbt_btstack_hid_event_result_t result =
        swbt_btstack_hid_event_decode(packet_type, packet, packet_size, out_event);
    if (result == SWBT_BTSTACK_HID_EVENT_OK) {
        return SWBT_BTSTACK_DEVICE_OK;
    }
    if (result == SWBT_BTSTACK_HID_EVENT_IGNORED) {
        return SWBT_BTSTACK_DEVICE_IGNORED;
    }
    return SWBT_BTSTACK_DEVICE_ERROR_RECV_FAILED;
}

void swbt_btstack_device_close(swbt_btstack_device_t *device) {
    if (device == NULL || !device->initialized) {
        return;
    }
    if (device->hid_registered) {
        device->port->hid_stop(device->port_context);
        device->hid_registered = false;
    }
    if (device->platform_started) {
        device->port->platform_stop(device->port_context);
        device->platform_started = false;
    }
}

bool swbt_btstack_device_is_open(const swbt_btstack_device_t *device) {
    return device != NULL && device->hid_registered;
}
