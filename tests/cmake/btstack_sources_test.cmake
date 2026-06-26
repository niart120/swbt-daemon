if(NOT DEFINED SWBT_SOURCE_DIR)
    message(FATAL_ERROR "SWBT_SOURCE_DIR is required")
endif()

set(SWBT_BTSTACK_DIR "${SWBT_SOURCE_DIR}/vendor/btstack")
include("${SWBT_SOURCE_DIR}/cmake/btstack_sources.cmake")

function(swbt_assert_list_contains list_var expected)
    list(FIND ${list_var} "${expected}" found_index)
    if(found_index EQUAL -1)
        message(FATAL_ERROR "${list_var} does not contain ${expected}")
    endif()
endfunction()

function(swbt_assert_list_excludes list_var unexpected)
    list(FIND ${list_var} "${unexpected}" found_index)
    if(NOT found_index EQUAL -1)
        message(FATAL_ERROR "${list_var} unexpectedly contains ${unexpected}")
    endif()
endfunction()

function(swbt_assert_sources_exist)
    foreach(source IN LISTS ARGN)
        if(NOT EXISTS "${source}")
            message(FATAL_ERROR "Selected BTstack source does not exist: ${source}")
        endif()
    endforeach()
endfunction()

swbt_collect_btstack_sources(
    BTSTACK_DIR "${SWBT_BTSTACK_DIR}"
    BACKEND libusb
    OUT_SOURCES libusb_sources
    OUT_INCLUDE_DIRS libusb_include_dirs
    OUT_LINK_LIBRARIES libusb_link_libraries
)

swbt_assert_sources_exist(${libusb_sources})
swbt_assert_list_contains(libusb_sources "${SWBT_BTSTACK_DIR}/src/hci.c")
swbt_assert_list_contains(libusb_sources "${SWBT_BTSTACK_DIR}/src/l2cap.c")
swbt_assert_list_contains(libusb_sources "${SWBT_BTSTACK_DIR}/src/classic/hid_device.c")
swbt_assert_list_contains(libusb_sources "${SWBT_BTSTACK_DIR}/src/classic/rfcomm.c")
swbt_assert_list_contains(libusb_sources "${SWBT_BTSTACK_DIR}/src/classic/sdp_server.c")
swbt_assert_list_contains(libusb_sources "${SWBT_BTSTACK_DIR}/platform/libusb/hci_transport_h2_libusb.c")
swbt_assert_list_contains(libusb_sources "${SWBT_BTSTACK_DIR}/platform/posix/btstack_run_loop_posix.c")
swbt_assert_list_contains(libusb_sources "${SWBT_BTSTACK_DIR}/platform/posix/btstack_tlv_posix.c")
swbt_assert_list_excludes(libusb_sources "${SWBT_BTSTACK_DIR}/platform/windows/hci_transport_h2_winusb.c")
swbt_assert_list_excludes(libusb_sources "${SWBT_BTSTACK_DIR}/port/libusb/main.c")
swbt_assert_list_contains(libusb_include_dirs "${SWBT_BTSTACK_DIR}/port/libusb")
swbt_assert_list_contains(libusb_include_dirs "${SWBT_BTSTACK_DIR}/src")
swbt_assert_list_contains(libusb_link_libraries "libusb-1.0")

swbt_collect_btstack_sources(
    BTSTACK_DIR "${SWBT_BTSTACK_DIR}"
    BACKEND windows-winusb
    OUT_SOURCES winusb_sources
    OUT_INCLUDE_DIRS winusb_include_dirs
    OUT_LINK_LIBRARIES winusb_link_libraries
)

swbt_assert_sources_exist(${winusb_sources})
swbt_assert_list_contains(winusb_sources "${SWBT_BTSTACK_DIR}/src/hci.c")
swbt_assert_list_contains(winusb_sources "${SWBT_BTSTACK_DIR}/src/l2cap.c")
swbt_assert_list_contains(winusb_sources "${SWBT_BTSTACK_DIR}/src/classic/hid_device.c")
swbt_assert_list_contains(winusb_sources "${SWBT_BTSTACK_DIR}/src/classic/rfcomm.c")
swbt_assert_list_contains(winusb_sources "${SWBT_BTSTACK_DIR}/src/classic/sdp_server.c")
swbt_assert_list_contains(winusb_sources "${SWBT_BTSTACK_DIR}/platform/windows/hci_transport_h2_winusb.c")
swbt_assert_list_contains(winusb_sources "${SWBT_BTSTACK_DIR}/platform/windows/btstack_run_loop_windows.c")
swbt_assert_list_contains(winusb_sources "${SWBT_BTSTACK_DIR}/platform/windows/btstack_tlv_windows.c")
swbt_assert_list_excludes(winusb_sources "${SWBT_BTSTACK_DIR}/platform/libusb/hci_transport_h2_libusb.c")
swbt_assert_list_excludes(winusb_sources "${SWBT_BTSTACK_DIR}/port/windows-winusb/main.c")
swbt_assert_list_contains(winusb_include_dirs "${SWBT_BTSTACK_DIR}/port/windows-winusb")
swbt_assert_list_contains(winusb_include_dirs "${SWBT_BTSTACK_DIR}/src")
swbt_assert_list_contains(winusb_link_libraries "setupapi")
swbt_assert_list_contains(winusb_link_libraries "winusb")
