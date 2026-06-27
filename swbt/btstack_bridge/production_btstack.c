#include "btstack_bridge/production_btstack.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "btstack_bridge/classic_discovery.h"
#include "btstack_bridge/classic_discovery_btstack_adapter.h"
#include "btstack_bridge/hci_dump_text.h"
#include "btstack_bridge/hid_device_btstack_adapter.h"
#include "btstack_bridge/hid_port.h"
#include "btstack_bridge/input_report_timer_adapter.h"
#include "btstack_bridge/output_report_callbacks.h"
#include "btstack_bridge/usb_adapter_location.h"
#include "btstack_memory.h"
#include "btstack_run_loop.h"
#include "btstack_tlv.h"
#include "btstack_util.h"
#include "classic/btstack_link_key_db_tlv.h"
#include "classic/hid_device.h"
#include "core/diagnostics.h"
#include "gap.h"
#include "hci_dump.h"
#include "hci.h"
#include "hci_transport_usb.h"
#include "l2cap.h"

#include <stdlib.h>

#if defined(SWBT_BACKEND_LIBUSB)
#include <libusb.h>
#endif

#if defined(SWBT_BACKEND_WINDOWS_WINUSB)
#include <windows.h>
#include <SetupAPI.h>
#ifndef SPDRP_LOCATION_PATHS
#define SPDRP_LOCATION_PATHS 0x00000023
#endif
#endif

#if defined(SWBT_BACKEND_WINDOWS_WINUSB)
int hci_transport_usb_set_location_path(const char *location_path);
#endif

#if defined(_WIN32)
#include "btstack_run_loop_windows.h"
#include "btstack_tlv_windows.h"
#else
#include "btstack_run_loop_posix.h"
#include "btstack_tlv_posix.h"
#endif

#define SWBT_BTSTACK_IPC_PUMP_PERIOD_MS 1u

static bool g_swbt_btstack_production_hci_dump_open;
static const char *g_hci_dump_path;
static const char *g_link_key_db_path;
static bool g_link_key_db_open;
static bool g_link_key_event_handler_registered;
static btstack_packet_callback_registration_t g_link_key_event_registration;
#if defined(_WIN32)
static btstack_tlv_windows_t g_link_key_tlv_context;
#else
static btstack_tlv_posix_t g_link_key_tlv_context;
#endif
static swbt_btstack_production_ipc_pump_t g_swbt_btstack_production_ipc_pump;
static bool g_swbt_btstack_production_ipc_pump_started;
static btstack_timer_source_t g_swbt_btstack_production_ipc_pump_timer;
static bool g_swbt_btstack_production_ipc_pump_timer_pending;

static void swbt_btstack_production_ipc_pump_schedule(void);

static void swbt_btstack_production_ipc_pump_timer_handler(btstack_timer_source_t *timer) {
    swbt_btstack_production_ipc_pump_t *pump = (swbt_btstack_production_ipc_pump_t *)timer->context;

    g_swbt_btstack_production_ipc_pump_timer_pending = false;
    if (pump != NULL && pump->is_running != NULL && pump->poll_once_at != NULL &&
        pump->is_running(pump->context)) {
        pump->poll_once_at(pump->context, btstack_run_loop_get_time_ms());
    }
    swbt_btstack_production_ipc_pump_schedule();
}

static void swbt_btstack_production_ipc_pump_schedule(void) {
    if (!g_swbt_btstack_production_ipc_pump_started ||
        g_swbt_btstack_production_ipc_pump_timer_pending) {
        return;
    }

    btstack_run_loop_set_timer(&g_swbt_btstack_production_ipc_pump_timer,
                               SWBT_BTSTACK_IPC_PUMP_PERIOD_MS);
    btstack_run_loop_add_timer(&g_swbt_btstack_production_ipc_pump_timer);
    g_swbt_btstack_production_ipc_pump_timer_pending = true;
}

static int swbt_btstack_production_ipc_pump_start(void *context,
                                                  const swbt_btstack_production_ipc_pump_t *pump) {
    (void)context;
    if (pump == NULL || pump->is_running == NULL || pump->poll_once_at == NULL) {
        return -1;
    }
    swbt_diagnostic_trace("btstack: ipc pump start");
    g_swbt_btstack_production_ipc_pump = *pump;
    g_swbt_btstack_production_ipc_pump_started = true;
    g_swbt_btstack_production_ipc_pump_timer_pending = false;
    btstack_run_loop_set_timer_handler(&g_swbt_btstack_production_ipc_pump_timer,
                                       swbt_btstack_production_ipc_pump_timer_handler);
    btstack_run_loop_set_timer_context(&g_swbt_btstack_production_ipc_pump_timer,
                                       &g_swbt_btstack_production_ipc_pump);
    swbt_diagnostic_trace("btstack: ipc pump start ok");
    return 0;
}

static void swbt_btstack_production_ipc_pump_stop(void *context) {
    (void)context;
    if (g_swbt_btstack_production_ipc_pump_timer_pending) {
        (void)btstack_run_loop_remove_timer(&g_swbt_btstack_production_ipc_pump_timer);
        g_swbt_btstack_production_ipc_pump_timer_pending = false;
    }
    g_swbt_btstack_production_ipc_pump_started = false;
    g_swbt_btstack_production_ipc_pump = (swbt_btstack_production_ipc_pump_t){0};
}

static swbt_btstack_classic_discovery_config_t swbt_btstack_production_discovery_config(void) {
    const swbt_btstack_hid_registration_config_t hid_config =
        swbt_btstack_production_hid_registration_config();
    return (swbt_btstack_classic_discovery_config_t){
        .class_of_device = hid_config.hid_device_subclass,
        .local_name = hid_config.device_name,
        .link_policy_settings =
            LM_LINK_POLICY_ENABLE_ROLE_SWITCH | LM_LINK_POLICY_ENABLE_SNIFF_MODE,
        .allow_role_switch = true,
        .discoverable = true,
    };
}

int swbt_btstack_production_hci_dump_start(const char *path) {
    swbt_btstack_hci_dump_text_result_t result;

    if (!swbt_diagnostic_path_is_enabled(path)) {
        return 0;
    }

    swbt_diagnostic_trace("btstack: hci dump open");
    result = swbt_btstack_hci_dump_text_open(path);
    if (result != SWBT_BTSTACK_HCI_DUMP_TEXT_OK) {
        swbt_diagnostic_trace("btstack: hci dump open failed");
        return -1;
    }
    hci_dump_init(swbt_btstack_hci_dump_text_instance());
    g_swbt_btstack_production_hci_dump_open = true;
    swbt_diagnostic_trace("btstack: hci dump open ok");
    return 0;
}

int swbt_btstack_production_hci_dump_configure(const char *path) {
    if (path != NULL && path[0] == '\0') {
        return -1;
    }
    g_hci_dump_path = path;
    return 0;
}

static void swbt_btstack_production_hci_dump_stop(void) {
    if (!g_swbt_btstack_production_hci_dump_open) {
        return;
    }

    swbt_diagnostic_trace("btstack: hci dump close");
    hci_dump_init(NULL);
    swbt_btstack_hci_dump_text_close();
    g_swbt_btstack_production_hci_dump_open = false;
    swbt_diagnostic_trace("btstack: hci dump close done");
}

int swbt_btstack_production_link_key_db_configure(const char *path) {
    if (path != NULL && path[0] == '\0') {
        return -1;
    }
    g_link_key_db_path = path;
    return 0;
}

#if defined(SWBT_BACKEND_WINDOWS_WINUSB)
static int swbt_btstack_production_winusb_location_path_set(void *context,
                                                            const char *location_path) {
    (void)context;
    return hci_transport_usb_set_location_path(location_path);
}
#endif

#if defined(SWBT_BACKEND_LIBUSB)
static void swbt_btstack_production_libusb_bus_and_path_set(void *context, uint8_t bus,
                                                            int port_count, uint8_t *ports) {
    (void)context;
    hci_transport_usb_set_bus_and_path(bus, port_count, ports);
}
#endif

int swbt_btstack_production_adapter_location_configure(const char *location) {
    swbt_btstack_usb_adapter_location_t parsed_location;
    swbt_btstack_usb_adapter_location_port_t port = {0};

    if (location == NULL || location[0] == '\0') {
        return -1;
    }
    if (swbt_btstack_usb_adapter_location_parse(location, &parsed_location) !=
        SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK) {
        return -1;
    }

#if defined(SWBT_BACKEND_WINDOWS_WINUSB)
    port.set_winusb_location_path = swbt_btstack_production_winusb_location_path_set;
#elif defined(SWBT_BACKEND_LIBUSB)
    port.set_libusb_bus_and_path = swbt_btstack_production_libusb_bus_and_path_set;
#else
    return -1;
#endif

    return swbt_btstack_usb_adapter_location_apply(&parsed_location, &port) ==
                   SWBT_BTSTACK_USB_ADAPTER_LOCATION_OK
               ? 0
               : -1;
}

#if defined(SWBT_BACKEND_LIBUSB)
static void swbt_btstack_production_write_uint(FILE *out, unsigned int value) {
    char buffer[16];

    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    if (out == NULL || snprintf(buffer, sizeof(buffer), "%u", value) < 0) {
        return;
    }
    fputs(buffer, out);
}

static void swbt_btstack_production_write_int(FILE *out, int value) {
    char buffer[16];

    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    if (out == NULL || snprintf(buffer, sizeof(buffer), "%d", value) < 0) {
        return;
    }
    fputs(buffer, out);
}

static void swbt_btstack_production_write_hex16(FILE *out, unsigned int value) {
    char buffer[8];

    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    if (out == NULL || snprintf(buffer, sizeof(buffer), "0x%04x", value & 0xffffu) < 0) {
        return;
    }
    fputs(buffer, out);
}

static void swbt_btstack_production_write_hex8(FILE *out, unsigned int value) {
    char buffer[6];

    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    if (out == NULL || snprintf(buffer, sizeof(buffer), "0x%02x", value & 0xffu) < 0) {
        return;
    }
    fputs(buffer, out);
}

static bool swbt_btstack_production_libusb_is_known_bluetooth_device(uint16_t vendor_id,
                                                                     uint16_t product_id) {
    static const uint16_t known_bluetooth_devices[] = {
        0x0a5c, 0x21e8, 0x0b05, 0x17cb, 0x0a5c, 0x22be, 0x2fe3, 0x0100, 0x2fe3, 0x000b,
    };

    for (size_t index = 0u; index + 1u < sizeof(known_bluetooth_devices) / sizeof(uint16_t);
         index += 2u) {
        if (known_bluetooth_devices[index] == vendor_id &&
            known_bluetooth_devices[index + 1u] == product_id) {
            return true;
        }
    }
    return false;
}

static bool swbt_btstack_production_libusb_is_bluetooth_device(
    const struct libusb_device_descriptor *descriptor) {
    if (descriptor == NULL) {
        return false;
    }
    return (descriptor->bDeviceClass == 0xE0u && descriptor->bDeviceSubClass == 0x01u &&
            descriptor->bDeviceProtocol == 0x01u) ||
           swbt_btstack_production_libusb_is_known_bluetooth_device(descriptor->idVendor,
                                                                    descriptor->idProduct);
}

static void
swbt_btstack_production_libusb_write_selector(FILE *out, libusb_device *device,
                                              const struct libusb_device_descriptor *descriptor,
                                              const uint8_t *ports, int port_count) {
    fputs("libusb:", out);
    swbt_btstack_production_write_uint(out, (unsigned int)libusb_get_bus_number(device));
    fputc(':', out);
    for (int index = 0; index < port_count; ++index) {
        if (index > 0) {
            fputc('.', out);
        }
        swbt_btstack_production_write_uint(out, (unsigned int)ports[index]);
    }
    fputs(" vid=", out);
    swbt_btstack_production_write_hex16(out, (unsigned int)descriptor->idVendor);
    fputs(" pid=", out);
    swbt_btstack_production_write_hex16(out, (unsigned int)descriptor->idProduct);
    fputs(" class=", out);
    swbt_btstack_production_write_hex8(out, (unsigned int)descriptor->bDeviceClass);
    fputs(" subclass=", out);
    swbt_btstack_production_write_hex8(out, (unsigned int)descriptor->bDeviceSubClass);
    fputs(" protocol=", out);
    swbt_btstack_production_write_hex8(out, (unsigned int)descriptor->bDeviceProtocol);
    fputc('\n', out);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): adapter inventory stream pair.
static int swbt_btstack_production_list_libusb_adapter_locations(FILE *out, FILE *err) {
    libusb_device **devices = NULL;
    ssize_t device_count = 0;
    int candidate_count = 0;
    int result = 0;

    if (out == NULL) {
        return -1;
    }
    result = libusb_init(NULL);
    if (result != 0) {
        if (err != NULL) {
            fputs("libusb_init failed: ", err);
            swbt_btstack_production_write_int(err, result);
            fputc('\n', err);
        }
        return -1;
    }
    device_count = libusb_get_device_list(NULL, &devices);
    if (device_count < 0) {
        if (err != NULL) {
            fputs("libusb_get_device_list failed: ", err);
            swbt_btstack_production_write_int(err, (int)device_count);
            fputc('\n', err);
        }
        libusb_exit(NULL);
        return -1;
    }

    for (ssize_t index = 0; index < device_count; ++index) {
        struct libusb_device_descriptor descriptor;
        uint8_t ports[SWBT_BTSTACK_USB_ADAPTER_LOCATION_MAX_PORTS] = {0};
        int port_count = 0;
        libusb_device *device = devices[index];

        if (libusb_get_device_descriptor(device, &descriptor) != 0 ||
            !swbt_btstack_production_libusb_is_bluetooth_device(&descriptor)) {
            continue;
        }
        port_count =
            libusb_get_port_numbers(device, ports, SWBT_BTSTACK_USB_ADAPTER_LOCATION_MAX_PORTS);
        if (port_count <= 0 || port_count > SWBT_BTSTACK_USB_ADAPTER_LOCATION_MAX_PORTS) {
            continue;
        }
        swbt_btstack_production_libusb_write_selector(out, device, &descriptor, ports, port_count);
        ++candidate_count;
    }

    if (candidate_count == 0) {
        fputs("No libusb Bluetooth adapter-location candidates found.\n", out);
    }
    libusb_free_device_list(devices, 1);
    libusb_exit(NULL);
    return 0;
}
#endif

#if defined(SWBT_BACKEND_WINDOWS_WINUSB)
static const GUID SWBT_GUID_DEVINTERFACE_USB_DEVICE = {
    0xA5DCBF10L,
    0x6530,
    0x11D2,
    {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED},
};

static int swbt_btstack_production_winusb_read_registry_property(HDEVINFO device_info,
                                                                 PSP_DEVINFO_DATA device_data,
                                                                 DWORD property, char **out_value,
                                                                 DWORD *out_property_type) {
    DWORD property_type = 0;
    DWORD required_size = 0;
    char *value = NULL;

    if (out_value == NULL || out_property_type == NULL) {
        return -1;
    }
    *out_value = NULL;
    *out_property_type = 0;
    if (SetupDiGetDeviceRegistryPropertyA(device_info, device_data, property, &property_type, NULL,
                                          0, &required_size)) {
        return 0;
    }
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || required_size == 0u) {
        return 0;
    }

    value = calloc((size_t)required_size + 2u, sizeof(char));
    if (value == NULL) {
        return -1;
    }
    if (!SetupDiGetDeviceRegistryPropertyA(device_info, device_data, property, &property_type,
                                           (PBYTE)value, required_size, NULL)) {
        free(value);
        return -1;
    }
    *out_value = value;
    *out_property_type = property_type;
    return 1;
}

static bool swbt_btstack_production_winusb_service_is_winusb(const char *service,
                                                             DWORD property_type) {
    return service != NULL && property_type == REG_SZ && strcmp(service, "WinUSB") == 0;
}

static void swbt_btstack_production_winusb_write_selector(FILE *out, const char *location_path,
                                                          const char *hardware_id) {
    fputs("winusb:", out);
    fputs(location_path, out);
    fputs(" service=WinUSB", out);
    if (hardware_id != NULL && hardware_id[0] != '\0') {
        fputs(" hardware_id=", out);
        fputs(hardware_id, out);
    }
    fputc('\n', out);
}

static int swbt_btstack_production_winusb_write_location_paths(FILE *out,
                                                               const char *location_paths,
                                                               DWORD property_type,
                                                               const char *hardware_id) {
    int count = 0;

    if (location_paths == NULL) {
        return 0;
    }
    if (property_type == REG_SZ) {
        swbt_btstack_production_winusb_write_selector(out, location_paths, hardware_id);
        return 1;
    }
    if (property_type != REG_MULTI_SZ) {
        return 0;
    }
    for (const char *cursor = location_paths; cursor[0] != '\0'; cursor += strlen(cursor) + 1u) {
        swbt_btstack_production_winusb_write_selector(out, cursor, hardware_id);
        ++count;
    }
    return count;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): adapter inventory stream pair.
static int swbt_btstack_production_list_winusb_adapter_locations(FILE *out, FILE *err) {
    HDEVINFO device_info = INVALID_HANDLE_VALUE;
    DWORD member_index = 0;
    int candidate_count = 0;

    if (out == NULL) {
        return -1;
    }

    device_info = SetupDiGetClassDevsA(&SWBT_GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL,
                                       DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (device_info == INVALID_HANDLE_VALUE) {
        if (err != NULL) {
            fputs("SetupDiGetClassDevsA failed\n", err);
        }
        return -1;
    }

    for (;;) {
        SP_DEVICE_INTERFACE_DATA interface_data;
        SP_DEVINFO_DATA device_data;
        PSP_DEVICE_INTERFACE_DETAIL_DATA_A detail_data = NULL;
        DWORD required_size = 0;
        char *service = NULL;
        char *location_paths = NULL;
        char *hardware_id = NULL;
        DWORD service_type = 0;
        DWORD location_type = 0;
        DWORD hardware_id_type = 0;

        memset(&interface_data, 0, sizeof(interface_data));
        interface_data.cbSize = sizeof(interface_data);
        if (!SetupDiEnumDeviceInterfaces(device_info, NULL,
                                         (LPGUID)&SWBT_GUID_DEVINTERFACE_USB_DEVICE, member_index,
                                         &interface_data)) {
            if (GetLastError() != ERROR_NO_MORE_ITEMS && err != NULL) {
                fputs("SetupDiEnumDeviceInterfaces failed\n", err);
            }
            break;
        }
        ++member_index;

        (void)SetupDiGetDeviceInterfaceDetailA(device_info, &interface_data, NULL, 0,
                                               &required_size, NULL);
        if (required_size == 0u) {
            continue;
        }
        detail_data = calloc(1u, required_size);
        if (detail_data == NULL) {
            SetupDiDestroyDeviceInfoList(device_info);
            return -1;
        }
        memset(&device_data, 0, sizeof(device_data));
        device_data.cbSize = sizeof(device_data);
        detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        if (!SetupDiGetDeviceInterfaceDetailA(device_info, &interface_data, detail_data,
                                              required_size, NULL, &device_data)) {
            free(detail_data);
            continue;
        }

        if (swbt_btstack_production_winusb_read_registry_property(
                device_info, &device_data, SPDRP_SERVICE, &service, &service_type) > 0 &&
            swbt_btstack_production_winusb_service_is_winusb(service, service_type) &&
            swbt_btstack_production_winusb_read_registry_property(
                device_info, &device_data, SPDRP_LOCATION_PATHS, &location_paths, &location_type) >
                0) {
            (void)swbt_btstack_production_winusb_read_registry_property(
                device_info, &device_data, SPDRP_HARDWAREID, &hardware_id, &hardware_id_type);
            (void)hardware_id_type;
            candidate_count += swbt_btstack_production_winusb_write_location_paths(
                out, location_paths, location_type, hardware_id);
        }

        free(hardware_id);
        free(location_paths);
        free(service);
        free(detail_data);
    }

    if (candidate_count == 0) {
        fputs("No WinUSB adapter-location candidates found.\n", out);
    }
    SetupDiDestroyDeviceInfoList(device_info);
    return 0;
}
#endif

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): public adapter inventory callback ABI.
int swbt_btstack_production_list_adapter_locations(void *context, FILE *out, FILE *err) {
    (void)context;
#if defined(SWBT_BACKEND_LIBUSB)
    return swbt_btstack_production_list_libusb_adapter_locations(out, err);
#elif defined(SWBT_BACKEND_WINDOWS_WINUSB)
    return swbt_btstack_production_list_winusb_adapter_locations(out, err);
#else
    (void)out;
    if (err != NULL) {
        fputs("adapter inventory is not supported by this build\n", err);
    }
    return -1;
#endif
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): BTstack packet handler ABI.
static void link_key_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet,
                                    uint16_t size) {
    bd_addr_t addr;
    link_key_t link_key;
    link_key_type_t link_key_type;
    (void)channel;

    if (!g_link_key_db_open || packet_type != HCI_EVENT_PACKET || packet == NULL || size < 25u ||
        packet[0] != HCI_EVENT_LINK_KEY_NOTIFICATION) {
        return;
    }
    if (btstack_is_null(&packet[8], sizeof(link_key))) {
        swbt_diagnostic_trace("btstack: link key db ignored null notification");
        return;
    }

    for (size_t index = 0u; index < sizeof(addr); ++index) {
        addr[index] = packet[7u - index];
    }
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    (void)memcpy(link_key, &packet[8], sizeof(link_key));
    link_key_type = (link_key_type_t)packet[24];
    gap_store_link_key_for_bd_addr(addr, link_key, link_key_type);
    swbt_diagnostic_trace("btstack: link key db stored notification");
}

static void link_key_events_start(void) {
    if (g_link_key_event_handler_registered) {
        return;
    }

    g_link_key_event_registration.callback = link_key_packet_handler;
    hci_add_event_handler(&g_link_key_event_registration);
    g_link_key_event_handler_registered = true;
}

static void link_key_events_stop(void) {
    if (!g_link_key_event_handler_registered) {
        return;
    }

    hci_remove_event_handler(&g_link_key_event_registration);
    g_link_key_event_handler_registered = false;
    g_link_key_event_registration = (btstack_packet_callback_registration_t){0};
}

static int link_key_db_start(void) {
    const btstack_tlv_t *tlv = NULL;
    const char *path = g_link_key_db_path;

    if (!swbt_diagnostic_path_is_enabled(path)) {
        return 0;
    }

    swbt_diagnostic_trace("btstack: link key db open");
#if defined(_WIN32)
    tlv = btstack_tlv_windows_init_instance(&g_link_key_tlv_context, path);
#else
    tlv = btstack_tlv_posix_init_instance(&g_link_key_tlv_context, path);
#endif
    if (tlv == NULL) {
        swbt_diagnostic_trace("btstack: link key db open failed");
        return -1;
    }

    btstack_tlv_set_instance(tlv, &g_link_key_tlv_context);
    hci_set_link_key_db(btstack_link_key_db_tlv_get_instance(tlv, &g_link_key_tlv_context));
    link_key_events_start();
    g_link_key_db_open = true;
    swbt_diagnostic_trace("btstack: link key db open ok");
    return 0;
}

static void link_key_db_stop(void) {
    if (!g_link_key_db_open) {
        return;
    }

    swbt_diagnostic_trace("btstack: link key db close");
    btstack_tlv_set_instance(NULL, NULL);
#if defined(_WIN32)
    btstack_tlv_windows_deinit(&g_link_key_tlv_context);
#else
    btstack_tlv_posix_deinit(&g_link_key_tlv_context);
#endif
    g_link_key_db_open = false;
    swbt_diagnostic_trace("btstack: link key db close done");
}

static int swbt_btstack_production_platform_start(void *context) {
    swbt_btstack_classic_discovery_result_t discovery_result;
    swbt_btstack_classic_discovery_config_t discovery_config;
    (void)context;
    if (swbt_btstack_production_hci_dump_start(g_hci_dump_path) != 0) {
        return -1;
    }
    swbt_diagnostic_trace("btstack: memory init");
    btstack_memory_init();
#if defined(_WIN32)
    swbt_diagnostic_trace("btstack: windows run loop init");
    btstack_run_loop_init(btstack_run_loop_windows_get_instance());
#else
    swbt_diagnostic_trace("btstack: posix run loop init");
    btstack_run_loop_init(btstack_run_loop_posix_get_instance());
#endif
    swbt_diagnostic_trace("btstack: hci init usb transport");
    hci_init(hci_transport_usb_instance(), NULL);
    if (link_key_db_start() != 0) {
        swbt_btstack_production_hci_dump_stop();
        return -1;
    }
    swbt_diagnostic_trace("btstack: classic discovery configure");
    discovery_config = swbt_btstack_production_discovery_config();
    discovery_result = swbt_btstack_classic_discovery_configure(
        swbt_btstack_classic_discovery_backend_btstack(), NULL, &discovery_config);
    if (discovery_result != SWBT_BTSTACK_CLASSIC_DISCOVERY_OK) {
        swbt_diagnostic_trace("btstack: classic discovery configure failed");
        link_key_events_stop();
        link_key_db_stop();
        swbt_btstack_production_hci_dump_stop();
        return -1;
    }
    swbt_diagnostic_trace("btstack: classic discovery configure ok");
    swbt_diagnostic_trace("btstack: l2cap init");
    l2cap_init();
    swbt_btstack_production_ipc_pump_schedule();
    return 0;
}

static void swbt_btstack_production_platform_stop(void *context) {
    (void)context;
    swbt_diagnostic_trace("btstack: ipc pump stop");
    swbt_btstack_production_ipc_pump_stop(NULL);
    swbt_diagnostic_trace("btstack: ipc pump stop done");
    link_key_events_stop();
    swbt_diagnostic_trace("btstack: hci close");
    hci_close();
    swbt_diagnostic_trace("btstack: hci close done");
    link_key_db_stop();
    swbt_diagnostic_trace("btstack: run loop deinit");
    btstack_run_loop_deinit();
    swbt_diagnostic_trace("btstack: run loop deinit done");
    swbt_btstack_production_hci_dump_stop();
}

static int
swbt_btstack_production_hid_register(void *context, uint8_t *service_buffer,
                                     size_t service_buffer_size,
                                     const swbt_btstack_hid_registration_config_t *config) {
    int result = 0;
    (void)context;
    swbt_diagnostic_trace("btstack: hid register");
    result = swbt_btstack_hid_device_register(swbt_btstack_hid_registration_backend_btstack(), NULL,
                                              service_buffer, service_buffer_size, config);
    swbt_diagnostic_trace(result == SWBT_BTSTACK_HID_REGISTRATION_OK
                              ? "btstack: hid register ok"
                              : "btstack: hid register failed");
    return result == SWBT_BTSTACK_HID_REGISTRATION_OK ? 0 : -1;
}

static void swbt_btstack_production_hid_stop(void *context) {
    (void)context;
    hid_device_deinit();
}

static void
swbt_btstack_production_output_handler_start(void *context,
                                             swbt_btstack_output_report_handler_t *handler) {
    (void)context;
    (void)swbt_btstack_output_report_callbacks_register(handler);
}

static void swbt_btstack_production_output_handler_stop(void *context) {
    (void)context;
    swbt_btstack_output_report_callbacks_unregister();
}

static int swbt_btstack_production_report_timer_init(
    void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
    const swbt_btstack_input_report_timer_adapter_config_t *config) {
    swbt_btstack_input_report_timer_adapter_config_t btstack_config;
    (void)context;
    if (config == NULL) {
        return -1;
    }
    btstack_config = *config;
    btstack_config.backend = swbt_btstack_input_report_timer_backend_btstack();
    return swbt_btstack_input_report_timer_adapter_init(adapter, &btstack_config) ==
                   SWBT_BTSTACK_INPUT_REPORT_TIMER_OK
               ? 0
               : -1;
}

static int swbt_btstack_production_report_timer_start(
    void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
    swbt_btstack_input_report_timer_start_options_t options) {
    (void)context;
    return swbt_btstack_input_report_timer_adapter_start(adapter, options) ==
                   SWBT_BTSTACK_INPUT_REPORT_TIMER_OK
               ? 0
               : -1;
}

static int swbt_btstack_production_report_timer_on_can_send_now(
    void *context, swbt_btstack_input_report_timer_adapter_t *adapter) {
    (void)context;
    return swbt_btstack_input_report_timer_adapter_on_can_send_now(adapter) ==
                   SWBT_BTSTACK_INPUT_REPORT_TIMER_OK
               ? 0
               : -1;
}

static int swbt_btstack_production_report_timer_enqueue_reply(
    void *context, swbt_btstack_input_report_timer_adapter_t *adapter, uint16_t hid_cid,
    const uint8_t *report, size_t report_size) {
    (void)context;
    return swbt_btstack_input_report_timer_adapter_enqueue_subcommand_reply(
               adapter, hid_cid, report, report_size) == SWBT_BTSTACK_INPUT_REPORT_TIMER_OK
               ? 0
               : -1;
}

static int swbt_btstack_production_report_timer_send_neutral_now(
    void *context, swbt_btstack_input_report_timer_adapter_t *adapter) {
    (void)context;
    const swbt_btstack_input_report_timer_result_t result =
        swbt_btstack_input_report_timer_adapter_send_neutral_now(adapter);
    if (result == SWBT_BTSTACK_INPUT_REPORT_TIMER_OK) {
        return 0;
    }
    if (result == SWBT_BTSTACK_INPUT_REPORT_TIMER_PENDING) {
        return 1;
    }
    return -1;
}

static void
swbt_btstack_production_report_timer_stop(void *context,
                                          swbt_btstack_input_report_timer_adapter_t *adapter) {
    (void)context;
    swbt_btstack_input_report_timer_adapter_stop(adapter);
}

static int swbt_btstack_production_ssp_confirm_user_confirmation(void *context,
                                                                 const uint8_t address[6]) {
    (void)context;
    if (address == NULL) {
        return -1;
    }
    return gap_ssp_confirmation_response(address);
}

static int swbt_btstack_production_read_controller_address(void *context, uint8_t address[6]) {
    (void)context;
    if (address == NULL) {
        return -1;
    }

    gap_local_bd_addr(address);
    return 0;
}

static uint32_t swbt_btstack_production_time_ms(void *context) {
    (void)context;
    return btstack_run_loop_get_time_ms();
}

static int swbt_btstack_production_power_on(void *context) {
    int result = 0;
    (void)context;
    swbt_diagnostic_trace("btstack: hci power on");
    result = hci_power_control(HCI_POWER_ON);
    swbt_diagnostic_trace(result == 0 ? "btstack: hci power on ok"
                                      : "btstack: hci power on failed");
    return result;
}

static void swbt_btstack_production_power_off(void *context) {
    (void)context;
    swbt_diagnostic_trace("btstack: hci power off");
    (void)hci_power_control(HCI_POWER_OFF);
}

static int swbt_btstack_production_device_connect(
    void *context, const swbt_btstack_device_connect_request_t *request, uint16_t *out_hid_cid) {
    uint8_t address[6];
    uint8_t status;
    (void)context;

    if (request == NULL || out_hid_cid == NULL ||
        request->control_psm != SWBT_BTSTACK_PRODUCTION_HID_CONTROL_PSM ||
        request->interrupt_psm != SWBT_BTSTACK_PRODUCTION_HID_INTERRUPT_PSM) {
        return -1;
    }

    for (size_t index = 0u; index < sizeof(address); ++index) {
        address[index] = request->address[index];
    }
    swbt_diagnostic_trace("btstack: hid active reconnect connect");
    status = hid_device_connect(address, out_hid_cid);
    swbt_diagnostic_trace(status == 0u ? "btstack: hid active reconnect connect ok"
                                       : "btstack: hid active reconnect connect failed");
    return status == 0u ? 0 : -1;
}

static int swbt_btstack_production_device_send(void *context, uint16_t hid_cid,
                                               const uint8_t *message, size_t message_size) {
    (void)context;
    return swbt_btstack_hid_port_send_report(swbt_btstack_hid_port_btstack(), hid_cid, message,
                                             message_size) == SWBT_BTSTACK_HID_PORT_OK
               ? 0
               : -1;
}

static void swbt_btstack_production_run_loop_execute(void *context) {
    (void)context;
    btstack_run_loop_execute();
}

static void swbt_btstack_production_run_loop_execute_on_main_thread(
    void *context, btstack_context_callback_registration_t *callback_registration) {
    (void)context;
    btstack_run_loop_execute_on_main_thread(callback_registration);
}

static void swbt_btstack_production_run_loop_trigger_exit(void *context) {
    (void)context;
    btstack_run_loop_trigger_exit();
}

const swbt_btstack_production_adapter_t *swbt_btstack_production_adapter(void) {
    static const swbt_btstack_production_adapter_t adapter = {
        .ipc_pump =
            {
                .start = swbt_btstack_production_ipc_pump_start,
                .stop = swbt_btstack_production_ipc_pump_stop,
            },
        .device =
            {
                .platform_start = swbt_btstack_production_platform_start,
                .platform_stop = swbt_btstack_production_platform_stop,
                .hid_register = swbt_btstack_production_hid_register,
                .hid_stop = swbt_btstack_production_hid_stop,
                .connect = swbt_btstack_production_device_connect,
                .send = swbt_btstack_production_device_send,
            },
        .output_handler =
            {
                .start = swbt_btstack_production_output_handler_start,
                .stop = swbt_btstack_production_output_handler_stop,
            },
        .report_timer =
            {
                .init = swbt_btstack_production_report_timer_init,
                .start = swbt_btstack_production_report_timer_start,
                .on_can_send_now = swbt_btstack_production_report_timer_on_can_send_now,
                .enqueue_subcommand_reply = swbt_btstack_production_report_timer_enqueue_reply,
                .send_neutral_now = swbt_btstack_production_report_timer_send_neutral_now,
                .stop = swbt_btstack_production_report_timer_stop,
            },
        .controller =
            {
                .confirm_ssp_user_confirmation =
                    swbt_btstack_production_ssp_confirm_user_confirmation,
                .read_controller_address = swbt_btstack_production_read_controller_address,
            },
        .clock =
            {
                .time_ms = swbt_btstack_production_time_ms,
            },
        .power =
            {
                .on = swbt_btstack_production_power_on,
                .off = swbt_btstack_production_power_off,
            },
        .run_loop =
            {
                .execute = swbt_btstack_production_run_loop_execute,
                .execute_on_main_thread = swbt_btstack_production_run_loop_execute_on_main_thread,
                .trigger_exit = swbt_btstack_production_run_loop_trigger_exit,
            },
    };
    return &adapter;
}
