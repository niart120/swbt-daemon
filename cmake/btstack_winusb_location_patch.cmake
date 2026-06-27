function(swbt_generate_btstack_winusb_location_transport input_path output_path)
    file(READ "${input_path}" swbt_winusb_transport_source)

    string(FIND "${swbt_winusb_transport_source}" "#include <Winusb.h>\n"
           swbt_winusb_include_index)
    if(swbt_winusb_include_index EQUAL -1)
        message(FATAL_ERROR "BTstack WinUSB transport include marker not found")
    endif()

    string(FIND "${swbt_winusb_transport_source}"
           "static btstack_linked_list_t usb_knwon_devices;\n\nvoid hci_transport_usb_add_device"
           swbt_winusb_known_devices_index)
    if(swbt_winusb_known_devices_index EQUAL -1)
        message(FATAL_ERROR "BTstack WinUSB known-device marker not found")
    endif()

    string(FIND "${swbt_winusb_transport_source}"
           "            // try all devices\n            BOOL result = usb_try_open_device(DevIntfDetailData->DevicePath);"
           swbt_winusb_try_all_index)
    if(swbt_winusb_try_all_index EQUAL -1)
        message(FATAL_ERROR "BTstack WinUSB open marker not found")
    endif()

    string(REPLACE
        "#include <Winusb.h>\n"
        [=[#include <Winusb.h>

#ifndef SPDRP_LOCATION_PATHS
#define SPDRP_LOCATION_PATHS 0x00000023
#endif

]=]
        swbt_winusb_transport_source
        "${swbt_winusb_transport_source}"
    )

    string(REPLACE
        "static btstack_linked_list_t usb_knwon_devices;\n\nvoid hci_transport_usb_add_device"
        [=[static btstack_linked_list_t usb_knwon_devices;
static char *usb_location_path;

int hci_transport_usb_set_location_path(const char *location_path) {
    if (usb_location_path != NULL) {
        HeapFree(GetProcessHeap(), 0, usb_location_path);
        usb_location_path = NULL;
    }
    if (location_path == NULL) {
        return 0;
    }
    if (location_path[0] == '\0') {
        return -1;
    }

    size_t location_path_len = strlen(location_path) + 1u;
    usb_location_path = HeapAlloc(GetProcessHeap(), 0, location_path_len);
    if (usb_location_path == NULL) {
        return -1;
    }
    memcpy(usb_location_path, location_path, location_path_len);
    return 0;
}

void hci_transport_usb_add_device]=]
        swbt_winusb_transport_source
        "${swbt_winusb_transport_source}"
    )

    string(REPLACE
        "    return false;\n}\n\n#ifdef ENABLE_SCO_OVER_HCI"
        [=[    return false;
}

static bool usb_location_path_list_matches(const char *location_paths) {
    const char *cursor = location_paths;
    if (usb_location_path == NULL) {
        return true;
    }
    if (location_paths == NULL) {
        return false;
    }

    while (cursor[0] != '\0') {
        if (strcmp(cursor, usb_location_path) == 0) {
            return true;
        }
        cursor += strlen(cursor) + 1u;
    }
    return false;
}

static bool usb_location_path_matches(HDEVINFO h_dev_info, PSP_DEVINFO_DATA dev_data) {
    DWORD property_type = 0;
    DWORD required_size = 0;
    char *location_paths = NULL;
    bool matches = false;

    if (usb_location_path == NULL) {
        return true;
    }
    if (h_dev_info == INVALID_HANDLE_VALUE || dev_data == NULL) {
        return false;
    }

    if (SetupDiGetDeviceRegistryPropertyA(h_dev_info, dev_data, SPDRP_LOCATION_PATHS,
                                          &property_type, NULL, 0, &required_size)) {
        return false;
    }
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || required_size == 0u) {
        return false;
    }

    location_paths = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, required_size + 2u);
    if (location_paths == NULL) {
        return false;
    }
    if (SetupDiGetDeviceRegistryPropertyA(h_dev_info, dev_data, SPDRP_LOCATION_PATHS,
                                          &property_type, (PBYTE)location_paths, required_size,
                                          NULL)) {
        if (property_type == REG_MULTI_SZ) {
            matches = usb_location_path_list_matches(location_paths);
        } else if (property_type == REG_SZ) {
            matches = strcmp(location_paths, usb_location_path) == 0;
        }
    }
    HeapFree(GetProcessHeap(), 0, location_paths);
    return matches;
}

#ifdef ENABLE_SCO_OVER_HCI]=]
        swbt_winusb_transport_source
        "${swbt_winusb_transport_source}"
    )

    string(REPLACE
        [=[            // try all devices
            BOOL result = usb_try_open_device(DevIntfDetailData->DevicePath);
            if (result){
                log_info("usb_open: Device opened, stop scanning");
                r = 0;
            } else {
                log_error("usb_open: Device open failed");
            }]=]
        [=[            if (!usb_location_path_matches(hDevInfo, &DevData)) {
                log_info("usb_open: Location Path does not match selector");
            } else {
                // try all matching devices
                BOOL result = usb_try_open_device(DevIntfDetailData->DevicePath);
                if (result){
                    log_info("usb_open: Device opened, stop scanning");
                    r = 0;
                } else {
                    log_error("usb_open: Device open failed");
                }
            }]=]
        swbt_winusb_transport_source
        "${swbt_winusb_transport_source}"
    )

    get_filename_component(swbt_winusb_output_dir "${output_path}" DIRECTORY)
    file(MAKE_DIRECTORY "${swbt_winusb_output_dir}")
    file(WRITE "${output_path}" "${swbt_winusb_transport_source}")
endfunction()
