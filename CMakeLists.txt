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
    LANGUAGES C
)

option(SWBT_BUILD_TESTS "Build unit tests" ON)
option(SWBT_BUILD_SHARED "Build shared C ABI library" ON)
option(SWBT_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(SWBT_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(SWBT_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
set(SWBT_BACKEND "libusb" CACHE STRING "Backend: libusb or windows-winusb")
set_property(CACHE SWBT_BACKEND PROPERTY STRINGS libusb windows-winusb)
set(SWBT_REPORT_PERIOD_US "8000" CACHE STRING "Default HID report period")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/compiler_warnings.cmake)
include(cmake/sanitizers.cmake)
include(cmake/btstack_sources.cmake)

set(SWBT_BTSTACK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/btstack" CACHE PATH "BTstack source directory")
if(EXISTS "${SWBT_BTSTACK_DIR}/LICENSE")
    message(STATUS "BTstack source: ${SWBT_BTSTACK_DIR}")
else()
    message(WARNING "BTstack submodule not found at ${SWBT_BTSTACK_DIR}. Run: git submodule update --init --recursive")
endif()
swbt_define_btstack_source_selection(swbt_btstack_source_selection)
get_property(swbt_btstack_selected_include_dirs TARGET swbt_btstack_source_selection
             PROPERTY SWBT_BTSTACK_SELECTED_INCLUDE_DIRS)

add_library(swbt_core STATIC
    swbt/core/swbt_version.c
    swbt/core/state_mailbox.c
    swbt/daemon/config.c
    swbt/daemon/runtime.c
    swbt/btstack_bridge/hid_device_btstack_adapter.c
    swbt/btstack_bridge/hid_device_registration.c
    swbt/btstack_bridge/output_report_callbacks.c
    swbt/btstack_bridge/input_report_scheduler.c
    swbt/btstack_bridge/input_report_timer_adapter.c
    swbt/btstack_bridge/output_report_handler.c
    swbt/btstack_bridge/subcommand_reply_queue.c
    swbt/ipc/ipc_json.c
    swbt/ipc/ipc_server.c
    swbt/ipc/ipc_session.c
    swbt/switch/switch_controller_state.c
    swbt/switch/switch_hid_descriptor.c
    swbt/switch/switch_report.c
    swbt/switch/switch_rumble.c
    swbt/switch/switch_spi.c
    swbt/switch/switch_spi_seed.c
    swbt/switch/switch_subcommand.c
    swbt/switch/switch_subcommand_dispatcher.c
    swbt/switch/switch_subcommand_reply.c
)

target_include_directories(swbt_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/swbt"
)

if(WIN32)
    target_link_libraries(swbt_core PUBLIC ws2_32)
endif()

target_include_directories(swbt_core PRIVATE ${swbt_btstack_selected_include_dirs})

swbt_apply_common_warnings(swbt_core)
swbt_apply_sanitizers(swbt_core)

if(SWBT_BUILD_SHARED)
    add_library(swbt SHARED
        api/swbt_c_api.c
    )
    target_link_libraries(swbt PRIVATE swbt_core)
    target_include_directories(swbt
        PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/api"
    )
    swbt_apply_common_warnings(swbt)
    swbt_apply_sanitizers(swbt)
endif()

add_executable(swbt-daemon
    apps/swbt-daemon/main.c
)
target_link_libraries(swbt-daemon PRIVATE swbt_core)
swbt_apply_common_warnings(swbt-daemon)
swbt_apply_sanitizers(swbt-daemon)

if(SWBT_BUILD_TESTS)
    enable_testing()

    add_executable(swbt_smoke_test
        tests/swbt_smoke_test.c
    )
    target_link_libraries(swbt_smoke_test PRIVATE swbt_core)
    swbt_apply_common_warnings(swbt_smoke_test)
    swbt_apply_sanitizers(swbt_smoke_test)

    add_test(NAME swbt_smoke_test COMMAND swbt_smoke_test)
    add_test(
        NAME btstack_sources_cmake_test
        COMMAND
            "${CMAKE_COMMAND}"
            "-DSWBT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/btstack_sources_test.cmake"
    )

    add_executable(switch_report_test
        tests/switch_report_test.c
    )
    target_link_libraries(switch_report_test PRIVATE swbt_core)
    swbt_apply_common_warnings(switch_report_test)
    swbt_apply_sanitizers(switch_report_test)

    add_test(NAME switch_report_test COMMAND switch_report_test)

    add_executable(switch_hid_descriptor_test
        tests/switch_hid_descriptor_test.c
    )
    target_link_libraries(switch_hid_descriptor_test PRIVATE swbt_core)
    swbt_apply_common_warnings(switch_hid_descriptor_test)
    swbt_apply_sanitizers(switch_hid_descriptor_test)

    add_test(NAME switch_hid_descriptor_test COMMAND switch_hid_descriptor_test)

    add_executable(switch_subcommand_test
        tests/switch_subcommand_test.c
    )
    target_link_libraries(switch_subcommand_test PRIVATE swbt_core)
    swbt_apply_common_warnings(switch_subcommand_test)
    swbt_apply_sanitizers(switch_subcommand_test)

    add_test(NAME switch_subcommand_test COMMAND switch_subcommand_test)

    add_executable(switch_subcommand_reply_test
        tests/switch_subcommand_reply_test.c
    )
    target_link_libraries(switch_subcommand_reply_test PRIVATE swbt_core)
    swbt_apply_common_warnings(switch_subcommand_reply_test)
    swbt_apply_sanitizers(switch_subcommand_reply_test)

    add_test(NAME switch_subcommand_reply_test COMMAND switch_subcommand_reply_test)

    add_executable(switch_subcommand_dispatcher_test
        tests/switch_subcommand_dispatcher_test.c
    )
    target_link_libraries(switch_subcommand_dispatcher_test PRIVATE swbt_core)
    swbt_apply_common_warnings(switch_subcommand_dispatcher_test)
    swbt_apply_sanitizers(switch_subcommand_dispatcher_test)

    add_test(NAME switch_subcommand_dispatcher_test COMMAND switch_subcommand_dispatcher_test)

    add_executable(switch_spi_test
        tests/switch_spi_test.c
    )
    target_link_libraries(switch_spi_test PRIVATE swbt_core)
    swbt_apply_common_warnings(switch_spi_test)
    swbt_apply_sanitizers(switch_spi_test)

    add_test(NAME switch_spi_test COMMAND switch_spi_test)

    add_executable(switch_spi_seed_test
        tests/switch_spi_seed_test.c
    )
    target_link_libraries(switch_spi_seed_test PRIVATE swbt_core)
    swbt_apply_common_warnings(switch_spi_seed_test)
    swbt_apply_sanitizers(switch_spi_seed_test)

    add_test(NAME switch_spi_seed_test COMMAND switch_spi_seed_test)

    add_executable(switch_rumble_test
        tests/switch_rumble_test.c
    )
    target_link_libraries(switch_rumble_test PRIVATE swbt_core)
    swbt_apply_common_warnings(switch_rumble_test)
    swbt_apply_sanitizers(switch_rumble_test)

    add_test(NAME switch_rumble_test COMMAND switch_rumble_test)

    add_executable(ipc_session_test
        tests/ipc_session_test.c
    )
    target_link_libraries(ipc_session_test PRIVATE swbt_core)
    swbt_apply_common_warnings(ipc_session_test)
    swbt_apply_sanitizers(ipc_session_test)

    add_test(NAME ipc_session_test COMMAND ipc_session_test)

    add_executable(state_mailbox_test
        tests/state_mailbox_test.c
    )
    target_link_libraries(state_mailbox_test PRIVATE swbt_core)
    swbt_apply_common_warnings(state_mailbox_test)
    swbt_apply_sanitizers(state_mailbox_test)

    add_test(NAME state_mailbox_test COMMAND state_mailbox_test)

    add_executable(ipc_json_test
        tests/ipc_json_test.c
    )
    target_link_libraries(ipc_json_test PRIVATE swbt_core)
    swbt_apply_common_warnings(ipc_json_test)
    swbt_apply_sanitizers(ipc_json_test)

    add_test(NAME ipc_json_test COMMAND ipc_json_test)

    add_executable(ipc_server_test
        tests/ipc_server_test.c
    )
    target_link_libraries(ipc_server_test PRIVATE swbt_core)
    swbt_apply_common_warnings(ipc_server_test)
    swbt_apply_sanitizers(ipc_server_test)

    add_test(NAME ipc_server_test COMMAND ipc_server_test)

    add_executable(btstack_hid_device_registration_test
        tests/btstack_hid_device_registration_test.c
    )
    target_link_libraries(btstack_hid_device_registration_test PRIVATE swbt_core)
    swbt_apply_common_warnings(btstack_hid_device_registration_test)
    swbt_apply_sanitizers(btstack_hid_device_registration_test)

    add_test(NAME btstack_hid_device_registration_test COMMAND btstack_hid_device_registration_test)

    add_executable(btstack_hid_device_btstack_adapter_test
        tests/btstack_hid_device_btstack_adapter_test.c
    )
    target_link_libraries(btstack_hid_device_btstack_adapter_test PRIVATE swbt_core)
    target_include_directories(btstack_hid_device_btstack_adapter_test
        PRIVATE
            ${swbt_btstack_selected_include_dirs}
    )
    swbt_apply_common_warnings(btstack_hid_device_btstack_adapter_test)
    swbt_apply_sanitizers(btstack_hid_device_btstack_adapter_test)

    add_test(NAME btstack_hid_device_btstack_adapter_test COMMAND btstack_hid_device_btstack_adapter_test)

    add_executable(btstack_output_report_handler_test
        tests/btstack_output_report_handler_test.c
    )
    target_link_libraries(btstack_output_report_handler_test PRIVATE swbt_core)
    swbt_apply_common_warnings(btstack_output_report_handler_test)
    swbt_apply_sanitizers(btstack_output_report_handler_test)

    add_test(NAME btstack_output_report_handler_test COMMAND btstack_output_report_handler_test)

    add_executable(btstack_output_report_callbacks_test
        tests/btstack_output_report_callbacks_test.c
    )
    target_link_libraries(btstack_output_report_callbacks_test PRIVATE swbt_core)
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
    target_link_libraries(btstack_subcommand_reply_queue_test PRIVATE swbt_core)
    swbt_apply_common_warnings(btstack_subcommand_reply_queue_test)
    swbt_apply_sanitizers(btstack_subcommand_reply_queue_test)

    add_test(NAME btstack_subcommand_reply_queue_test COMMAND btstack_subcommand_reply_queue_test)

    add_executable(btstack_input_report_timer_adapter_test
        tests/btstack_input_report_timer_adapter_test.c
    )
    target_link_libraries(btstack_input_report_timer_adapter_test PRIVATE swbt_core)
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
    target_link_libraries(btstack_input_report_scheduler_test PRIVATE swbt_core)
    swbt_apply_common_warnings(btstack_input_report_scheduler_test)
    swbt_apply_sanitizers(btstack_input_report_scheduler_test)

    add_test(NAME btstack_input_report_scheduler_test COMMAND btstack_input_report_scheduler_test)

    add_executable(daemon_runtime_test
        tests/daemon_runtime_test.c
    )
    target_link_libraries(daemon_runtime_test PRIVATE swbt_core)
    swbt_apply_common_warnings(daemon_runtime_test)
    swbt_apply_sanitizers(daemon_runtime_test)

    add_test(NAME daemon_runtime_test COMMAND daemon_runtime_test)
endif()
