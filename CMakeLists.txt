cmake_minimum_required(VERSION 3.25)

option(SWBT_ALLOW_HOST_BUILD "Allow unsupported local builds outside the Dev Container" OFF)

set(SWBT_IN_CI OFF)
if(DEFINED ENV{CI} AND NOT "$ENV{CI}" STREQUAL "" AND NOT "$ENV{CI}" STREQUAL "0")
    set(SWBT_IN_CI ON)
endif()

set(SWBT_IN_DEVCONTAINER OFF)
if(DEFINED ENV{SWBT_DEVCONTAINER} AND "$ENV{SWBT_DEVCONTAINER}" STREQUAL "1")
    set(SWBT_IN_DEVCONTAINER ON)
endif()

set(SWBT_ALLOW_HOST_BUILD_ENV OFF)
if(DEFINED ENV{SWBT_ALLOW_HOST_BUILD} AND NOT "$ENV{SWBT_ALLOW_HOST_BUILD}" STREQUAL "" AND NOT "$ENV{SWBT_ALLOW_HOST_BUILD}" STREQUAL "0")
    set(SWBT_ALLOW_HOST_BUILD_ENV ON)
endif()

if(NOT SWBT_IN_CI AND NOT SWBT_IN_DEVCONTAINER AND NOT SWBT_ALLOW_HOST_BUILD AND NOT SWBT_ALLOW_HOST_BUILD_ENV)
    message(FATAL_ERROR
        "Local CMake builds are supported only inside the Dev Container. "
        "Open this repository in the Dev Container, or pass -DSWBT_ALLOW_HOST_BUILD=ON "
        "for an explicit unsupported host build."
    )
endif()

project(swbt_daemon
    VERSION 0.1.0
    DESCRIPTION "Nintendo Switch Bluetooth controller daemon built on BTstack"
    LANGUAGES C CXX
)

option(SWBT_BUILD_TESTS "Build unit tests" ON)
option(SWBT_BUILD_SHARED "Build shared C ABI library" ON)
option(SWBT_ENABLE_CCACHE "Use ccache as the C/C++ compiler launcher when available" ON)
option(SWBT_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(SWBT_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(SWBT_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
set(SWBT_BACKEND "libusb" CACHE STRING "Backend: libusb or windows-winusb")
set_property(CACHE SWBT_BACKEND PROPERTY STRINGS libusb windows-winusb)
set(SWBT_REPORT_PERIOD_US "8000" CACHE STRING "Default HID report period")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(SWBT_ENABLE_CCACHE)
    find_program(SWBT_CCACHE_PROGRAM ccache)
    if(SWBT_CCACHE_PROGRAM)
        message(STATUS "ccache launcher: ${SWBT_CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${SWBT_CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${SWBT_CCACHE_PROGRAM}")
    else()
        message(STATUS "ccache launcher: not found")
    endif()
endif()

if(MINGW)
    add_link_options(-static)
endif()

include(cmake/compiler_warnings.cmake)
include(cmake/sanitizers.cmake)
include(cmake/btstack_sources.cmake)
include(cmake/btstack_winusb_location_patch.cmake)
include(cmake/module_public_includes.cmake)

set(SWBT_TOML11_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/toml11" CACHE PATH "toml11 source directory")
if(EXISTS "${SWBT_TOML11_DIR}/LICENSE")
    message(STATUS "toml11 source: ${SWBT_TOML11_DIR}")
else()
    message(WARNING "toml11 submodule not found at ${SWBT_TOML11_DIR}. Run: git submodule update --init --recursive")
endif()

add_library(swbt_toml11 INTERFACE)
target_include_directories(swbt_toml11
    SYSTEM
    INTERFACE
        "${SWBT_TOML11_DIR}/include"
)
target_compile_features(swbt_toml11 INTERFACE cxx_std_17)

set(SWBT_BTSTACK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/btstack" CACHE PATH "BTstack source directory")
if(EXISTS "${SWBT_BTSTACK_DIR}/LICENSE")
    message(STATUS "BTstack source: ${SWBT_BTSTACK_DIR}")
else()
    message(WARNING "BTstack submodule not found at ${SWBT_BTSTACK_DIR}. Run: git submodule update --init --recursive")
endif()
swbt_define_btstack_source_selection(swbt_btstack_source_selection)
get_property(swbt_btstack_selected_include_dirs TARGET swbt_btstack_source_selection
             PROPERTY SWBT_BTSTACK_SELECTED_INCLUDE_DIRS)
get_property(swbt_btstack_selected_sources TARGET swbt_btstack_source_selection
             PROPERTY SWBT_BTSTACK_SELECTED_SOURCES)
get_property(swbt_btstack_selected_link_libraries TARGET swbt_btstack_source_selection
             PROPERTY SWBT_BTSTACK_SELECTED_LINK_LIBRARIES)

set(swbt_btstack_link_sources ${swbt_btstack_selected_sources})
list(FILTER swbt_btstack_link_sources EXCLUDE REGEX ".*/platform/(posix|libusb|windows)/.*")
set(swbt_btstack_link_include_dirs ${swbt_btstack_selected_include_dirs})
set(swbt_btstack_link_libraries ${swbt_btstack_selected_link_libraries})
if(SWBT_BACKEND STREQUAL "libusb")
    list(APPEND swbt_btstack_link_sources
        "${SWBT_BTSTACK_DIR}/platform/libusb/hci_transport_h2_libusb.c"
        "${SWBT_BTSTACK_DIR}/platform/posix/btstack_run_loop_posix.c"
        "${SWBT_BTSTACK_DIR}/platform/posix/btstack_tlv_posix.c"
    )
    find_path(swbt_libusb_include_dir NAMES libusb.h PATH_SUFFIXES libusb-1.0)
    if(swbt_libusb_include_dir)
        list(APPEND swbt_btstack_link_include_dirs "${swbt_libusb_include_dir}")
    endif()
    find_library(swbt_libusb_library NAMES usb-1.0)
    if(swbt_libusb_library)
        list(REMOVE_ITEM swbt_btstack_link_libraries libusb-1.0)
        list(APPEND swbt_btstack_link_libraries "${swbt_libusb_library}")
    endif()
elseif(SWBT_BACKEND STREQUAL "windows-winusb")
    set(swbt_btstack_winusb_transport_source
        "${CMAKE_CURRENT_BINARY_DIR}/generated/btstack/platform/windows/hci_transport_h2_winusb_location.c"
    )
    swbt_generate_btstack_winusb_location_transport(
        "${SWBT_BTSTACK_DIR}/platform/windows/hci_transport_h2_winusb.c"
        "${swbt_btstack_winusb_transport_source}"
    )
    list(APPEND swbt_btstack_link_sources
        "${swbt_btstack_winusb_transport_source}"
        "${SWBT_BTSTACK_DIR}/platform/windows/btstack_run_loop_windows.c"
        "${SWBT_BTSTACK_DIR}/platform/windows/btstack_tlv_windows.c"
    )
    if(MINGW)
        list(APPEND swbt_btstack_link_include_dirs
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/mingw-compat-include"
        )
    endif()
endif()
list(REMOVE_DUPLICATES swbt_btstack_link_sources)
list(REMOVE_DUPLICATES swbt_btstack_link_include_dirs)

add_library(swbt_btstack STATIC
    ${swbt_btstack_link_sources}
)
target_include_directories(swbt_btstack
    PUBLIC
        ${swbt_btstack_link_include_dirs}
)
target_link_libraries(swbt_btstack
    PUBLIC
        ${swbt_btstack_link_libraries}
)

add_library(swbt_switch_protocol STATIC
    swbt/switch/switch_controller_state.c
    swbt/switch/switch_device_info.c
    swbt/switch/switch_hid_descriptor.c
    swbt/switch/switch_report.c
    swbt/switch/switch_player_lights.c
    swbt/switch/switch_rumble.c
    swbt/switch/switch_spi.c
    swbt/switch/switch_spi_seed.c
    swbt/switch/switch_subcommand.c
    swbt/switch/switch_subcommand_dispatcher.c
    swbt/switch/switch_subcommand_reply.c
)
swbt_configure_module_public_includes(swbt_switch_protocol MODULES switch)
swbt_apply_common_warnings(swbt_switch_protocol)
swbt_apply_sanitizers(swbt_switch_protocol)

add_library(swbt_support STATIC
    swbt/support/swbt_version.c
    swbt/support/diagnostics.c
    swbt/support/logging.c
    swbt/support/metrics.c
)
swbt_configure_module_public_includes(swbt_support MODULES support)
swbt_apply_common_warnings(swbt_support)
swbt_apply_sanitizers(swbt_support)

add_library(swbt_domain STATIC
    swbt/domain/domain.c
    swbt/domain/lease.c
)
swbt_configure_module_public_includes(swbt_domain MODULES domain)
target_link_libraries(swbt_domain PUBLIC swbt_support swbt_switch_protocol)
swbt_apply_common_warnings(swbt_domain)
swbt_apply_sanitizers(swbt_domain)

add_library(swbt_btstack_bridge STATIC
    swbt/btstack_bridge/production_ports.c
    swbt/btstack_bridge/usb_adapter_location.c
    swbt/btstack_bridge/classic_discovery.c
    swbt/btstack_bridge/classic_discovery_btstack_adapter.c
    swbt/btstack_bridge/device.c
    swbt/btstack_bridge/hci_dump_text.c
    swbt/btstack_bridge/hid_event.c
    swbt/btstack_bridge/hid_port.c
    swbt/btstack_bridge/hid_device_btstack_adapter.c
    swbt/btstack_bridge/hid_device_registration.c
    swbt/btstack_bridge/output_report_callbacks.c
    swbt/btstack_bridge/input_report_scheduler.c
    swbt/btstack_bridge/input_report_timer_adapter.c
    swbt/btstack_bridge/output_report_handler.c
    swbt/btstack_bridge/subcommand_reply_queue.c
    swbt/btstack_bridge/timer_port.c
)
swbt_configure_module_public_includes(swbt_btstack_bridge MODULES btstack_bridge)
target_include_directories(swbt_btstack_bridge PUBLIC ${swbt_btstack_selected_include_dirs})
target_link_libraries(swbt_btstack_bridge PUBLIC swbt_domain swbt_switch_protocol swbt_support)
swbt_apply_common_warnings(swbt_btstack_bridge)
swbt_apply_sanitizers(swbt_btstack_bridge)

add_library(swbt_runtime STATIC
    swbt/runtime/host.c
)
swbt_configure_module_public_includes(swbt_runtime MODULES runtime)
target_link_libraries(swbt_runtime PUBLIC swbt_domain swbt_btstack_bridge swbt_support)
swbt_apply_common_warnings(swbt_runtime)
swbt_apply_sanitizers(swbt_runtime)

add_library(swbt_control STATIC
    swbt/control/control.c
)
swbt_configure_module_public_includes(swbt_control MODULES control)
target_link_libraries(swbt_control PUBLIC swbt_domain swbt_runtime)
swbt_apply_common_warnings(swbt_control)
swbt_apply_sanitizers(swbt_control)

set_target_properties(
    swbt_switch_protocol
    swbt_support
    swbt_domain
    swbt_btstack_bridge
    swbt_runtime
    swbt_control
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)

add_library(swbt_ipc STATIC
    swbt/ipc/ipc_adapter.c
    swbt/ipc/ipc_json.c
    swbt/ipc/ipc_server.c
)
swbt_configure_module_public_includes(swbt_ipc MODULES ipc)
if(WIN32)
    target_link_libraries(swbt_ipc PUBLIC ws2_32)
endif()
target_link_libraries(swbt_ipc PUBLIC swbt_control swbt_support)
swbt_apply_common_warnings(swbt_ipc)
swbt_apply_sanitizers(swbt_ipc)

add_library(swbt_daemon_process STATIC
    swbt/daemon/config.c
    swbt/daemon/config_file.cpp
    swbt/daemon/cli.c
    swbt/daemon/ipc_runner.c
    swbt/daemon/launch_options.c
    swbt/daemon/production_hid_session.c
    swbt/daemon/production_ipc_pump.c
    swbt/daemon/production_process_backend.c
    swbt/daemon/production_report_timer.c
    swbt/daemon/production_reconnect.c
    swbt/daemon/production_shutdown.c
    swbt/daemon/production_runner.c
    swbt/daemon/process.c
    swbt/daemon/switch_address.c
)
swbt_configure_module_public_includes(swbt_daemon_process MODULES daemon)
target_link_libraries(swbt_daemon_process
    PUBLIC
        swbt_ipc
        swbt_runtime
        swbt_btstack_bridge
        swbt_domain
        swbt_support
    PRIVATE
        swbt_toml11
)
swbt_apply_common_warnings(swbt_daemon_process)
swbt_apply_sanitizers(swbt_daemon_process)

add_library(swbt_btstack_production_impl STATIC
    swbt/btstack_bridge/production_btstack_impl.c
)
target_include_directories(swbt_btstack_production_impl
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/swbt"
    PRIVATE
        ${swbt_btstack_selected_include_dirs}
)
target_link_libraries(swbt_btstack_production_impl PUBLIC swbt_btstack_bridge swbt_support swbt_btstack)
if(SWBT_BACKEND STREQUAL "libusb")
    target_compile_definitions(swbt_btstack_production_impl PRIVATE SWBT_BACKEND_LIBUSB=1)
elseif(SWBT_BACKEND STREQUAL "windows-winusb")
    target_compile_definitions(swbt_btstack_production_impl PRIVATE SWBT_BACKEND_WINDOWS_WINUSB=1)
endif()
swbt_apply_common_warnings(swbt_btstack_production_impl)
swbt_apply_sanitizers(swbt_btstack_production_impl)

if(SWBT_BUILD_SHARED)
    add_library(swbt SHARED
        api/swbt_c_api.c
    )
    target_link_libraries(swbt PRIVATE swbt_control swbt_support)
    target_include_directories(swbt
        PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/api"
    )
    swbt_apply_common_warnings(swbt)
    swbt_apply_sanitizers(swbt)
endif()

add_executable(swbt-daemon
    apps/swbt-daemon/main.c
    apps/swbt-daemon/platform_process.c
    apps/swbt-daemon/production_entrypoint.c
)
target_link_libraries(swbt-daemon PRIVATE swbt_daemon_process swbt_btstack_production_impl)
if(WIN32)
    target_link_libraries(swbt-daemon PRIVATE dbghelp)
endif()
target_include_directories(swbt-daemon
    PRIVATE
        ${swbt_btstack_selected_include_dirs}
)
swbt_apply_common_warnings(swbt-daemon)
swbt_apply_sanitizers(swbt-daemon)

add_executable(swbt-debug-client
    apps/swbt-debug-client/debug_client.c
    apps/swbt-debug-client/main.c
)
target_link_libraries(swbt-debug-client PRIVATE swbt_ipc)
swbt_apply_common_warnings(swbt-debug-client)
swbt_apply_sanitizers(swbt-debug-client)

if(SWBT_BUILD_TESTS)
    enable_testing()

    add_executable(swbt_smoke_test
        tests/swbt_smoke_test.c
    )
    target_link_libraries(swbt_smoke_test PRIVATE swbt_support swbt_switch_protocol)
    swbt_apply_common_warnings(swbt_smoke_test)
    swbt_apply_sanitizers(swbt_smoke_test)

    add_test(NAME swbt_smoke_test COMMAND swbt_smoke_test)
    add_test(NAME swbt_daemon_noop_smoke_test COMMAND swbt-daemon --backend noop)
    add_test(NAME swbt_daemon_help_test COMMAND swbt-daemon help)
    add_test(NAME swbt_daemon_config_smoke_test COMMAND swbt-daemon config --backend noop)
    add_test(
        NAME btstack_sources_cmake_test
        COMMAND
            "${CMAKE_COMMAND}"
            "-DSWBT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/btstack_sources_test.cmake"
    )
    add_test(
        NAME include_boundaries_cmake_test
        COMMAND
            "${CMAKE_COMMAND}"
            "-DSWBT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/include_boundaries_test.cmake"
    )
    add_test(
        NAME compile_include_boundaries_cmake_test
        COMMAND
            "${CMAKE_COMMAND}"
            "-DSWBT_BINARY_DIR=${CMAKE_BINARY_DIR}"
            "-DSWBT_C_COMPILER=${CMAKE_C_COMPILER}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/compile_include_boundaries_test.cmake"
    )
    add_test(
        NAME architecture_absence_cmake_test
        COMMAND
            "${CMAKE_COMMAND}"
            "-DSWBT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/architecture_absence_test.cmake"
    )
    add_test(
        NAME production_entrypoint_boundary_cmake_test
        COMMAND
            "${CMAKE_COMMAND}"
            "-DSWBT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/production_entrypoint_boundary_test.cmake"
    )

    add_executable(switch_report_test
        tests/switch_report_test.c
    )
    target_link_libraries(switch_report_test PRIVATE swbt_switch_protocol)
    swbt_apply_common_warnings(switch_report_test)
    swbt_apply_sanitizers(switch_report_test)

    add_test(NAME switch_report_test COMMAND switch_report_test)

    add_executable(switch_hid_descriptor_test
        tests/switch_hid_descriptor_test.c
    )
    target_link_libraries(switch_hid_descriptor_test PRIVATE swbt_switch_protocol)
    swbt_apply_common_warnings(switch_hid_descriptor_test)
    swbt_apply_sanitizers(switch_hid_descriptor_test)

    add_test(NAME switch_hid_descriptor_test COMMAND switch_hid_descriptor_test)

    add_executable(switch_subcommand_test
        tests/switch_subcommand_test.c
    )
    target_link_libraries(switch_subcommand_test PRIVATE swbt_switch_protocol)
    swbt_apply_common_warnings(switch_subcommand_test)
    swbt_apply_sanitizers(switch_subcommand_test)

    add_test(NAME switch_subcommand_test COMMAND switch_subcommand_test)

    add_executable(switch_subcommand_reply_test
        tests/switch_subcommand_reply_test.c
    )
    target_link_libraries(switch_subcommand_reply_test PRIVATE swbt_switch_protocol)
    swbt_apply_common_warnings(switch_subcommand_reply_test)
    swbt_apply_sanitizers(switch_subcommand_reply_test)

    add_test(NAME switch_subcommand_reply_test COMMAND switch_subcommand_reply_test)

    add_executable(switch_subcommand_dispatcher_test
        tests/switch_subcommand_dispatcher_test.c
    )
    target_link_libraries(switch_subcommand_dispatcher_test PRIVATE swbt_switch_protocol)
    swbt_apply_common_warnings(switch_subcommand_dispatcher_test)
    swbt_apply_sanitizers(switch_subcommand_dispatcher_test)

    add_test(NAME switch_subcommand_dispatcher_test COMMAND switch_subcommand_dispatcher_test)

    add_executable(switch_spi_test
        tests/switch_spi_test.c
    )
    target_link_libraries(switch_spi_test PRIVATE swbt_switch_protocol)
    swbt_apply_common_warnings(switch_spi_test)
    swbt_apply_sanitizers(switch_spi_test)

    add_test(NAME switch_spi_test COMMAND switch_spi_test)

    add_executable(switch_spi_seed_test
        tests/switch_spi_seed_test.c
    )
    target_link_libraries(switch_spi_seed_test PRIVATE swbt_switch_protocol)
    swbt_apply_common_warnings(switch_spi_seed_test)
    swbt_apply_sanitizers(switch_spi_seed_test)

    add_test(NAME switch_spi_seed_test COMMAND switch_spi_seed_test)

    add_executable(switch_rumble_test
        tests/switch_rumble_test.c
    )
    target_link_libraries(switch_rumble_test PRIVATE swbt_switch_protocol)
    swbt_apply_common_warnings(switch_rumble_test)
    swbt_apply_sanitizers(switch_rumble_test)

    add_test(NAME switch_rumble_test COMMAND switch_rumble_test)

    add_executable(switch_player_lights_test
        tests/switch_player_lights_test.c
    )
    target_link_libraries(switch_player_lights_test PRIVATE swbt_switch_protocol)
    swbt_apply_common_warnings(switch_player_lights_test)
    swbt_apply_sanitizers(switch_player_lights_test)

    add_test(NAME switch_player_lights_test COMMAND switch_player_lights_test)

    add_executable(domain_lease_test
        tests/domain_lease_test.c
    )
    target_link_libraries(domain_lease_test PRIVATE swbt_domain)
    swbt_apply_common_warnings(domain_lease_test)
    swbt_apply_sanitizers(domain_lease_test)

    add_test(NAME domain_lease_test COMMAND domain_lease_test)

    add_executable(domain_command_test
        tests/domain_command_test.c
    )
    target_link_libraries(domain_command_test PRIVATE swbt_domain)
    swbt_apply_common_warnings(domain_command_test)
    swbt_apply_sanitizers(domain_command_test)

    add_test(NAME domain_command_test COMMAND domain_command_test)

    add_executable(control_test
        tests/control_test.c
    )
    target_link_libraries(control_test PRIVATE swbt_control)
    swbt_apply_common_warnings(control_test)
    swbt_apply_sanitizers(control_test)

    add_test(NAME control_test COMMAND control_test)

    add_executable(ipc_json_test
        tests/ipc_json_test.c
    )
    target_link_libraries(ipc_json_test PRIVATE swbt_ipc)
    swbt_apply_common_warnings(ipc_json_test)
    swbt_apply_sanitizers(ipc_json_test)

    add_test(NAME ipc_json_test COMMAND ipc_json_test)

    add_executable(ipc_server_test
        tests/ipc_server_test.c
    )
    target_link_libraries(ipc_server_test PRIVATE swbt_ipc)
    swbt_apply_common_warnings(ipc_server_test)
    swbt_apply_sanitizers(ipc_server_test)

    add_test(NAME ipc_server_test COMMAND ipc_server_test)

    add_executable(debug_ipc_client_test
        apps/swbt-debug-client/debug_client.c
        tests/debug_ipc_client_test.c
    )
    target_link_libraries(debug_ipc_client_test PRIVATE swbt_ipc)
    swbt_apply_common_warnings(debug_ipc_client_test)
    swbt_apply_sanitizers(debug_ipc_client_test)

    add_test(NAME debug_ipc_client_test COMMAND debug_ipc_client_test)

    add_executable(btstack_hid_device_registration_test
        tests/btstack_hid_device_registration_test.c
    )
    target_link_libraries(btstack_hid_device_registration_test PRIVATE swbt_btstack_bridge)
    swbt_apply_common_warnings(btstack_hid_device_registration_test)
    swbt_apply_sanitizers(btstack_hid_device_registration_test)

    add_test(NAME btstack_hid_device_registration_test COMMAND btstack_hid_device_registration_test)

    add_executable(btstack_hid_device_btstack_adapter_test
        tests/btstack_hid_device_btstack_adapter_test.c
    )
    target_link_libraries(btstack_hid_device_btstack_adapter_test PRIVATE swbt_btstack_bridge)
    target_include_directories(btstack_hid_device_btstack_adapter_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(btstack_hid_device_btstack_adapter_test)
    swbt_apply_sanitizers(btstack_hid_device_btstack_adapter_test)

    add_test(NAME btstack_hid_device_btstack_adapter_test COMMAND btstack_hid_device_btstack_adapter_test)

    add_executable(btstack_hid_event_test
        tests/btstack_hid_event_test.c
    )
    target_link_libraries(btstack_hid_event_test PRIVATE swbt_btstack_bridge)
    target_include_directories(btstack_hid_event_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(btstack_hid_event_test)
    swbt_apply_sanitizers(btstack_hid_event_test)

    add_test(NAME btstack_hid_event_test COMMAND btstack_hid_event_test)

    add_executable(btstack_hid_port_test
        tests/btstack_hid_port_test.c
    )
    target_link_libraries(btstack_hid_port_test PRIVATE swbt_btstack_bridge)
    target_include_directories(btstack_hid_port_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(btstack_hid_port_test)
    swbt_apply_sanitizers(btstack_hid_port_test)

    add_test(NAME btstack_hid_port_test COMMAND btstack_hid_port_test)

    add_executable(btstack_device_test
        tests/btstack_device_test.c
    )
    target_link_libraries(btstack_device_test PRIVATE swbt_btstack_bridge)
    target_include_directories(btstack_device_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(btstack_device_test)
    swbt_apply_sanitizers(btstack_device_test)

    add_test(NAME btstack_device_test COMMAND btstack_device_test)

    add_executable(btstack_timer_port_test
        tests/btstack_timer_port_test.c
    )
    target_link_libraries(btstack_timer_port_test PRIVATE swbt_btstack_bridge)
    target_include_directories(btstack_timer_port_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(btstack_timer_port_test)
    swbt_apply_sanitizers(btstack_timer_port_test)

    add_test(NAME btstack_timer_port_test COMMAND btstack_timer_port_test)

    add_executable(btstack_classic_discovery_test
        tests/btstack_classic_discovery_test.c
    )
    target_link_libraries(btstack_classic_discovery_test PRIVATE swbt_btstack_bridge)
    swbt_apply_common_warnings(btstack_classic_discovery_test)
    swbt_apply_sanitizers(btstack_classic_discovery_test)

    add_test(NAME btstack_classic_discovery_test COMMAND btstack_classic_discovery_test)

    add_executable(btstack_hci_dump_text_test
        tests/btstack_hci_dump_text_test.c
    )
    target_link_libraries(btstack_hci_dump_text_test PRIVATE swbt_btstack_bridge)
    target_include_directories(btstack_hci_dump_text_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(btstack_hci_dump_text_test)
    swbt_apply_sanitizers(btstack_hci_dump_text_test)

    add_test(NAME btstack_hci_dump_text_test COMMAND btstack_hci_dump_text_test)

    add_executable(btstack_usb_adapter_location_test
        tests/btstack_usb_adapter_location_test.c
    )
    target_link_libraries(btstack_usb_adapter_location_test PRIVATE swbt_btstack_bridge)
    swbt_apply_common_warnings(btstack_usb_adapter_location_test)
    swbt_apply_sanitizers(btstack_usb_adapter_location_test)

    add_test(NAME btstack_usb_adapter_location_test COMMAND btstack_usb_adapter_location_test)

    add_executable(btstack_production_hci_dump_test
        tests/btstack_production_hci_dump_test.c
    )
    target_link_libraries(btstack_production_hci_dump_test PRIVATE swbt_btstack_production_impl)
    target_include_directories(btstack_production_hci_dump_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(btstack_production_hci_dump_test)
    swbt_apply_sanitizers(btstack_production_hci_dump_test)

    add_test(NAME btstack_production_hci_dump_test COMMAND btstack_production_hci_dump_test)

    add_executable(btstack_production_ports_test
        tests/btstack_production_ports_test.c
    )
    target_link_libraries(btstack_production_ports_test PRIVATE swbt_btstack_bridge)
    swbt_apply_common_warnings(btstack_production_ports_test)
    swbt_apply_sanitizers(btstack_production_ports_test)

    add_test(NAME btstack_production_ports_test COMMAND btstack_production_ports_test)

    add_executable(btstack_output_report_handler_test
        tests/btstack_output_report_handler_test.c
    )
    target_link_libraries(btstack_output_report_handler_test PRIVATE swbt_btstack_bridge)
    swbt_apply_common_warnings(btstack_output_report_handler_test)
    swbt_apply_sanitizers(btstack_output_report_handler_test)

    add_test(NAME btstack_output_report_handler_test COMMAND btstack_output_report_handler_test)

    add_executable(btstack_output_report_callbacks_test
        tests/btstack_output_report_callbacks_test.c
    )
    target_link_libraries(btstack_output_report_callbacks_test PRIVATE swbt_btstack_bridge)
    target_include_directories(btstack_output_report_callbacks_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(btstack_output_report_callbacks_test)
    swbt_apply_sanitizers(btstack_output_report_callbacks_test)

    add_test(NAME btstack_output_report_callbacks_test COMMAND btstack_output_report_callbacks_test)

    add_executable(btstack_subcommand_reply_queue_test
        tests/btstack_subcommand_reply_queue_test.c
    )
    target_link_libraries(btstack_subcommand_reply_queue_test PRIVATE swbt_btstack_bridge)
    swbt_apply_common_warnings(btstack_subcommand_reply_queue_test)
    swbt_apply_sanitizers(btstack_subcommand_reply_queue_test)

    add_test(NAME btstack_subcommand_reply_queue_test COMMAND btstack_subcommand_reply_queue_test)

    add_executable(btstack_input_report_timer_adapter_test
        tests/btstack_input_report_timer_adapter_test.c
    )
    target_link_libraries(btstack_input_report_timer_adapter_test PRIVATE swbt_btstack_bridge)
    target_include_directories(btstack_input_report_timer_adapter_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(btstack_input_report_timer_adapter_test)
    swbt_apply_sanitizers(btstack_input_report_timer_adapter_test)

    add_test(NAME btstack_input_report_timer_adapter_test COMMAND btstack_input_report_timer_adapter_test)

    add_executable(btstack_input_report_scheduler_test
        tests/btstack_input_report_scheduler_test.c
    )
    target_link_libraries(btstack_input_report_scheduler_test PRIVATE swbt_btstack_bridge)
    swbt_apply_common_warnings(btstack_input_report_scheduler_test)
    swbt_apply_sanitizers(btstack_input_report_scheduler_test)

    add_test(NAME btstack_input_report_scheduler_test COMMAND btstack_input_report_scheduler_test)

    add_executable(daemon_process_test
        tests/daemon_process_test.c
    )
    target_link_libraries(daemon_process_test PRIVATE swbt_daemon_process)
    swbt_apply_common_warnings(daemon_process_test)
    swbt_apply_sanitizers(daemon_process_test)

    add_test(NAME daemon_process_test COMMAND daemon_process_test)

    add_executable(runtime_host_test
        tests/runtime_host_test.c
    )
    target_link_libraries(runtime_host_test PRIVATE swbt_runtime)
    swbt_apply_common_warnings(runtime_host_test)
    swbt_apply_sanitizers(runtime_host_test)

    add_test(NAME runtime_host_test COMMAND runtime_host_test)

    add_executable(daemon_config_file_test
        tests/daemon_config_file_test.c
    )
    target_link_libraries(daemon_config_file_test PRIVATE swbt_daemon_process)
    swbt_apply_common_warnings(daemon_config_file_test)
    swbt_apply_sanitizers(daemon_config_file_test)

    add_test(NAME daemon_config_file_test COMMAND daemon_config_file_test)

    add_executable(daemon_switch_address_test
        tests/daemon_switch_address_test.c
    )
    target_link_libraries(daemon_switch_address_test PRIVATE swbt_daemon_process)
    swbt_apply_common_warnings(daemon_switch_address_test)
    swbt_apply_sanitizers(daemon_switch_address_test)

    add_test(NAME daemon_switch_address_test COMMAND daemon_switch_address_test)

    add_executable(daemon_production_reconnect_test
        tests/daemon_production_reconnect_test.c
    )
    target_link_libraries(daemon_production_reconnect_test PRIVATE swbt_daemon_process)
    swbt_apply_common_warnings(daemon_production_reconnect_test)
    swbt_apply_sanitizers(daemon_production_reconnect_test)

    add_test(NAME daemon_production_reconnect_test COMMAND daemon_production_reconnect_test)

    add_executable(daemon_production_ipc_pump_test
        tests/daemon_production_ipc_pump_test.c
    )
    target_link_libraries(daemon_production_ipc_pump_test PRIVATE swbt_daemon_process)
    swbt_apply_common_warnings(daemon_production_ipc_pump_test)
    swbt_apply_sanitizers(daemon_production_ipc_pump_test)

    add_test(NAME daemon_production_ipc_pump_test COMMAND daemon_production_ipc_pump_test)

    add_executable(daemon_launch_options_test
        tests/daemon_launch_options_test.c
    )
    target_link_libraries(daemon_launch_options_test PRIVATE swbt_daemon_process)
    swbt_apply_common_warnings(daemon_launch_options_test)
    swbt_apply_sanitizers(daemon_launch_options_test)

    add_test(NAME daemon_launch_options_test COMMAND daemon_launch_options_test)

    add_executable(daemon_cli_test
        tests/daemon_cli_test.c
    )
    target_link_libraries(daemon_cli_test PRIVATE swbt_daemon_process)
    swbt_apply_common_warnings(daemon_cli_test)
    swbt_apply_sanitizers(daemon_cli_test)

    add_test(NAME daemon_cli_test COMMAND daemon_cli_test)

    add_executable(architecture_journey_test
        tests/architecture_journey_test.c
    )
    target_link_libraries(architecture_journey_test PRIVATE swbt_daemon_process)
    swbt_apply_common_warnings(architecture_journey_test)
    swbt_apply_sanitizers(architecture_journey_test)

    add_test(NAME architecture_journey_test COMMAND architecture_journey_test)

    add_executable(daemon_ipc_runner_test
        apps/swbt-debug-client/debug_client.c
        tests/daemon_ipc_runner_test.c
    )
    target_link_libraries(daemon_ipc_runner_test PRIVATE swbt_daemon_process)
    swbt_apply_common_warnings(daemon_ipc_runner_test)
    swbt_apply_sanitizers(daemon_ipc_runner_test)

    add_test(NAME daemon_ipc_runner_test COMMAND daemon_ipc_runner_test)

    add_executable(daemon_production_runner_test
        tests/daemon_production_runner_test.c
    )
    target_link_libraries(daemon_production_runner_test PRIVATE swbt_daemon_process)
    target_include_directories(daemon_production_runner_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(daemon_production_runner_test)
    swbt_apply_sanitizers(daemon_production_runner_test)

    add_test(NAME daemon_production_runner_test COMMAND daemon_production_runner_test)

    add_executable(daemon_production_process_backend_test
        tests/daemon_production_process_backend_test.c
    )
    target_link_libraries(daemon_production_process_backend_test PRIVATE swbt_daemon_process)
    target_include_directories(daemon_production_process_backend_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(daemon_production_process_backend_test)
    swbt_apply_sanitizers(daemon_production_process_backend_test)

    add_test(NAME daemon_production_process_backend_test COMMAND daemon_production_process_backend_test)

    add_executable(daemon_production_shutdown_test
        tests/daemon_production_shutdown_test.c
    )
    target_link_libraries(daemon_production_shutdown_test PRIVATE swbt_daemon_process)
    target_include_directories(daemon_production_shutdown_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(daemon_production_shutdown_test)
    swbt_apply_sanitizers(daemon_production_shutdown_test)

    add_test(NAME daemon_production_shutdown_test COMMAND daemon_production_shutdown_test)

    add_executable(daemon_production_hid_session_test
        tests/daemon_production_hid_session_test.c
    )
    target_link_libraries(daemon_production_hid_session_test PRIVATE swbt_daemon_process)
    target_include_directories(daemon_production_hid_session_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(daemon_production_hid_session_test)
    swbt_apply_sanitizers(daemon_production_hid_session_test)

    add_test(NAME daemon_production_hid_session_test COMMAND daemon_production_hid_session_test)

    add_executable(daemon_production_report_timer_test
        tests/daemon_production_report_timer_test.c
    )
    target_link_libraries(daemon_production_report_timer_test PRIVATE swbt_daemon_process)
    target_include_directories(daemon_production_report_timer_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(daemon_production_report_timer_test)
    swbt_apply_sanitizers(daemon_production_report_timer_test)

    add_test(NAME daemon_production_report_timer_test COMMAND daemon_production_report_timer_test)

    add_executable(daemon_production_hid_sdp_record_test
        tests/daemon_production_hid_sdp_record_test.c
    )
    target_link_libraries(daemon_production_hid_sdp_record_test PRIVATE swbt_daemon_process swbt_btstack)
    target_include_directories(daemon_production_hid_sdp_record_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(daemon_production_hid_sdp_record_test)
    swbt_apply_sanitizers(daemon_production_hid_sdp_record_test)

    add_test(NAME daemon_production_hid_sdp_record_test COMMAND daemon_production_hid_sdp_record_test)

    add_executable(report_metrics_test
        tests/report_metrics_test.c
    )
    target_link_libraries(report_metrics_test PRIVATE swbt_support)
    swbt_apply_common_warnings(report_metrics_test)
    swbt_apply_sanitizers(report_metrics_test)

    add_test(NAME report_metrics_test COMMAND report_metrics_test)

    add_executable(diagnostics_test
        tests/diagnostics_test.c
    )
    target_link_libraries(diagnostics_test PRIVATE swbt_support)
    swbt_apply_common_warnings(diagnostics_test)
    swbt_apply_sanitizers(diagnostics_test)

    add_test(NAME diagnostics_test COMMAND diagnostics_test)

    if(SWBT_BUILD_SHARED)
        add_executable(swbt_c_api_test
            tests/swbt_c_api_test.c
        )
        target_link_libraries(swbt_c_api_test PRIVATE swbt)
        swbt_apply_common_warnings(swbt_c_api_test)
        swbt_apply_sanitizers(swbt_c_api_test)

        add_test(NAME swbt_c_api_test COMMAND swbt_c_api_test)
    endif()

    add_custom_target(swbt_unit_tests
        DEPENDS
            swbt_smoke_test
            switch_report_test
            switch_hid_descriptor_test
            switch_subcommand_test
            switch_subcommand_reply_test
            switch_subcommand_dispatcher_test
            switch_spi_test
            switch_spi_seed_test
            switch_rumble_test
            switch_player_lights_test
            domain_lease_test
            domain_command_test
            control_test
            ipc_json_test
            ipc_server_test
            debug_ipc_client_test
            btstack_hid_device_registration_test
            btstack_hid_device_btstack_adapter_test
            btstack_hid_event_test
            btstack_hid_port_test
            btstack_device_test
            btstack_timer_port_test
            btstack_classic_discovery_test
            btstack_hci_dump_text_test
            btstack_usb_adapter_location_test
            btstack_production_hci_dump_test
            btstack_production_ports_test
            btstack_output_report_handler_test
            btstack_output_report_callbacks_test
            btstack_subcommand_reply_queue_test
            btstack_input_report_timer_adapter_test
            btstack_input_report_scheduler_test
            daemon_process_test
            runtime_host_test
            daemon_config_file_test
            daemon_switch_address_test
            daemon_production_reconnect_test
            daemon_production_ipc_pump_test
            daemon_launch_options_test
            daemon_cli_test
            architecture_journey_test
            daemon_ipc_runner_test
            daemon_production_runner_test
            daemon_production_process_backend_test
            daemon_production_shutdown_test
            daemon_production_hid_session_test
            daemon_production_report_timer_test
            daemon_production_hid_sdp_record_test
            report_metrics_test
            diagnostics_test
            swbt-daemon
    )
    if(SWBT_BUILD_SHARED)
        add_dependencies(swbt_unit_tests swbt_c_api_test)
    endif()
endif()
