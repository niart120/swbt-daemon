#include "btstack_bridge/usb_adapter_location.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *winusb_location_path;
    uint8_t libusb_bus;
    int libusb_port_count;
    uint8_t libusb_ports[SWBT_BTSTACK_USB_ADAPTER_LOCATION_MAX_PORTS];
    int winusb_calls;
    int libusb_calls;
    int winusb_result;
} fake_usb_location_selector_t;

static int expect_eq_int(int actual, int expected, const char *message) {
    if (actual == expected) {
        return 0;
    }
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    fprintf(stderr, "%s: expected %d, got %d\n", message, expected, actual);
    return 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected, const char *message) {
    if (actual == expected) {
        return 0;
    }
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    fprintf(stderr, "%s: expected %u, got %u\n", message, expected, actual);
    return 1;
}

static int expect_str_eq(const char *actual, const char *expected, const char *message) {
    if (actual != NULL && expected != NULL && strcmp(actual, expected) == 0) {
        return 0;
    }
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    fprintf(stderr, "%s: expected %s, got %s\n", message, expected,
            actual == NULL ? "(null)" : actual);
    return 1;
}

static int fake_winusb_location_path_set(void *context, const char *location_path) {
    fake_usb_location_selector_t *selector = context;
    if (selector == NULL) {
        return -1;
    }
    selector->winusb_calls += 1;
    selector->winusb_location_path = location_path;
    return selector->winusb_result;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): callback ABI.
static void fake_libusb_bus_and_path_set(void *context, uint8_t bus, int port_count,
                                         uint8_t *ports) {
    fake_usb_location_selector_t *selector = context;
    if (selector == NULL) {
        return;
    }
    selector->libusb_calls += 1;
    selector->libusb_bus = bus;
    selector->libusb_port_count = port_count;
    for (int index = 0; index < port_count; ++index) {
        selector->libusb_ports[index] = ports[index];
    }
}

static int libusb_location_parse_accepts_bus_and_port_path(void) {
    swbt_btstack_usb_adapter_location_t location = {0};

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_usb_adapter_location_parse("libusb:12:3.2.1", &location),
                            SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK, "parse");
    failed += expect_eq_int((int)location.backend,
                            (int)SWBT_BTSTACK_USB_ADAPTER_LOCATION_BACKEND_LIBUSB, "backend");
    failed += expect_eq_u8(location.libusb_bus, 12u, "bus");
    failed += expect_eq_int(location.libusb_port_count, 3, "port count");
    failed += expect_eq_u8(location.libusb_ports[0], 3u, "port 0");
    failed += expect_eq_u8(location.libusb_ports[1], 2u, "port 1");
    failed += expect_eq_u8(location.libusb_ports[2], 1u, "port 2");
    return failed;
}

static int winusb_location_parse_keeps_location_path_suffix(void) {
    swbt_btstack_usb_adapter_location_t location = {0};

    int failed = 0;
    failed += expect_eq_int(
        swbt_btstack_usb_adapter_location_parse("winusb:PCIROOT(0)#USBROOT(0)#USB(3)", &location),
        SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK, "parse");
    failed += expect_eq_int((int)location.backend,
                            (int)SWBT_BTSTACK_USB_ADAPTER_LOCATION_BACKEND_WINUSB, "backend");
    failed += expect_str_eq(location.winusb_location_path, "PCIROOT(0)#USBROOT(0)#USB(3)",
                            "location path");
    return failed;
}

static int libusb_location_apply_calls_bus_and_path_port(void) {
    fake_usb_location_selector_t selector = {0};
    swbt_btstack_usb_adapter_location_t location = {0};
    const swbt_btstack_usb_adapter_location_port_t port = {
        .set_libusb_bus_and_path = fake_libusb_bus_and_path_set,
        .context = &selector,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_usb_adapter_location_parse("libusb:1:7.6", &location),
                            SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK, "parse");
    failed += expect_eq_int(swbt_btstack_usb_adapter_location_apply(&location, &port),
                            SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK, "apply");
    failed += expect_eq_int(selector.libusb_calls, 1, "libusb calls");
    failed += expect_eq_u8(selector.libusb_bus, 1u, "bus");
    failed += expect_eq_int(selector.libusb_port_count, 2, "port count");
    failed += expect_eq_u8(selector.libusb_ports[0], 7u, "port 0");
    failed += expect_eq_u8(selector.libusb_ports[1], 6u, "port 1");
    failed += expect_eq_int(selector.winusb_calls, 0, "winusb calls");
    return failed;
}

static int winusb_location_apply_calls_location_path_port(void) {
    fake_usb_location_selector_t selector = {0};
    swbt_btstack_usb_adapter_location_t location = {0};
    const swbt_btstack_usb_adapter_location_port_t port = {
        .set_winusb_location_path = fake_winusb_location_path_set,
        .context = &selector,
    };

    int failed = 0;
    failed += expect_eq_int(
        swbt_btstack_usb_adapter_location_parse("winusb:PCIROOT(0)#USBROOT(0)#USB(4)", &location),
        SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK, "parse");
    failed += expect_eq_int(swbt_btstack_usb_adapter_location_apply(&location, &port),
                            SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK, "apply");
    failed += expect_eq_int(selector.winusb_calls, 1, "winusb calls");
    failed += expect_str_eq(selector.winusb_location_path, "PCIROOT(0)#USBROOT(0)#USB(4)",
                            "location path");
    failed += expect_eq_int(selector.libusb_calls, 0, "libusb calls");
    return failed;
}

static int location_apply_rejects_unsupported_port(void) {
    fake_usb_location_selector_t selector = {0};
    swbt_btstack_usb_adapter_location_t location = {0};
    const swbt_btstack_usb_adapter_location_port_t port = {
        .set_libusb_bus_and_path = fake_libusb_bus_and_path_set,
        .context = &selector,
    };

    int failed = 0;
    failed += expect_eq_int(
        swbt_btstack_usb_adapter_location_parse("winusb:PCIROOT(0)#USBROOT(0)#USB(4)", &location),
        SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK, "parse");
    failed += expect_eq_int(swbt_btstack_usb_adapter_location_apply(&location, &port),
                            SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_UNSUPPORTED_BACKEND, "apply");
    failed += expect_eq_int(selector.winusb_calls, 0, "winusb calls");
    failed += expect_eq_int(selector.libusb_calls, 0, "libusb calls");
    return failed;
}

static int location_parse_rejects_malformed_values(void) {
    const char *const invalid_values[] = {
        "winusb:",
        "libusb:1",
        "libusb:1:",
        "libusb:0:3",
        "libusb:256:3",
        "libusb:1:0",
        "libusb:1:256",
        "libusb:1:3.",
        "libusb:1:.3",
        "libusb:1:3..2",
        "libusb:1:3.2.4.5.6.7.8.9",
        "bluetooth:1:3",
    };
    int failed = 0;
    for (size_t index = 0u; index < sizeof(invalid_values) / sizeof(invalid_values[0]); ++index) {
        swbt_btstack_usb_adapter_location_t location = {0};
        failed +=
            expect_eq_int(swbt_btstack_usb_adapter_location_parse(invalid_values[index], &location),
                          SWBT_BTSTACK_USB_ADAPTER_LOCATION_ERROR_INVALID_FORMAT, "parse invalid");
    }
    return failed;
}

int main(void) {
    int failed = 0;
    failed += libusb_location_parse_accepts_bus_and_port_path();
    failed += winusb_location_parse_keeps_location_path_suffix();
    failed += libusb_location_apply_calls_bus_and_path_port();
    failed += winusb_location_apply_calls_location_path_port();
    failed += location_apply_rejects_unsupported_port();
    failed += location_parse_rejects_malformed_values();
    return failed == 0 ? 0 : 1;
}
