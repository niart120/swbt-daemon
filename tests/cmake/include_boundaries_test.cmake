if(NOT DEFINED SWBT_SOURCE_DIR)
    message(FATAL_ERROR "SWBT_SOURCE_DIR is required")
endif()

function(swbt_assert_file_not_match relative_path pattern label)
    set(path "${SWBT_SOURCE_DIR}/${relative_path}")
    file(READ "${path}" content)
    if(content MATCHES "${pattern}")
        message(FATAL_ERROR "${label}: ${relative_path} matched forbidden pattern ${pattern}")
    endif()
endfunction()

function(swbt_assert_file_match relative_path pattern label)
    set(path "${SWBT_SOURCE_DIR}/${relative_path}")
    file(READ "${path}" content)
    if(NOT content MATCHES "${pattern}")
        message(FATAL_ERROR "${label}: ${relative_path} did not match required pattern ${pattern}")
    endif()
endfunction()

# Target sources still receive private source-tree include paths. These absence checks cover
# forbidden implementation includes that public include-root compile probes cannot detect.
file(GLOB_RECURSE swbt_application_files
    "${SWBT_SOURCE_DIR}/swbt/application/*.c"
    "${SWBT_SOURCE_DIR}/swbt/application/*.h"
)
foreach(path IN LISTS swbt_application_files)
    file(RELATIVE_PATH relative_path "${SWBT_SOURCE_DIR}" "${path}")
    swbt_assert_file_not_match("${relative_path}" "#include \"(btstack_bridge|classic|daemon|ipc)/"
                               "application boundary")
    swbt_assert_file_not_match("${relative_path}" "#include \"(bluetooth|btstack|gap|hci|l2cap)"
                               "application BTstack header boundary")
endforeach()

file(GLOB_RECURSE swbt_protocol_files
    "${SWBT_SOURCE_DIR}/swbt/switch/*.c"
    "${SWBT_SOURCE_DIR}/swbt/switch/*.h"
)
foreach(path IN LISTS swbt_protocol_files)
    file(RELATIVE_PATH relative_path "${SWBT_SOURCE_DIR}" "${path}")
    swbt_assert_file_not_match("${relative_path}" "#include \"(application|btstack_bridge|core|daemon|ipc)/"
                               "protocol boundary")
    swbt_assert_file_not_match("${relative_path}" "#include \"(bluetooth|btstack|classic|gap|hci|l2cap)"
                               "protocol BTstack header boundary")
endforeach()

file(GLOB_RECURSE swbt_btstack_bridge_files
    "${SWBT_SOURCE_DIR}/swbt/btstack_bridge/*.c"
    "${SWBT_SOURCE_DIR}/swbt/btstack_bridge/*.h"
)
foreach(path IN LISTS swbt_btstack_bridge_files)
    file(RELATIVE_PATH relative_path "${SWBT_SOURCE_DIR}" "${path}")
    swbt_assert_file_not_match("${relative_path}"
                               "#include \"(ipc/|daemon/(host|ipc_runner)\\.h)"
                               "BTstack bridge IPC internal include boundary")
endforeach()

swbt_assert_file_not_match("swbt/btstack_bridge/production_btstack.c"
                           "#include \"daemon/ipc_runner.h\""
                           "BTstack production adapter IPC runner include boundary")
swbt_assert_file_not_match("swbt/btstack_bridge/production_btstack.c"
                           "swbt_daemon_ipc_runner"
                           "BTstack production adapter IPC runner type boundary")
swbt_assert_file_not_match("swbt/btstack_bridge/production_btstack.h"
                           "daemon/production_backend.h"
                           "BTstack production adapter production backend struct boundary")

# These checks cover CMake topology. Public include-root compile probes cover include visibility,
# but they do not prove the intended target names and unit-test link targets remain in place.
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_application STATIC"
                       "application target boundary")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_ipc STATIC"
                       "IPC target boundary")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_btstack_adapter STATIC"
                       "BTstack adapter target boundary")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_daemon_host STATIC"
                       "daemon host target boundary")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt SHARED"
                       "public C ABI target boundary")
swbt_assert_file_not_match("CMakeLists.txt"
                           "target_link_libraries\\(swbt[ \r\n][^\\)]*swbt_ipc"
                           "public C ABI IPC link boundary")
swbt_assert_file_not_match("CMakeLists.txt"
                           "target_link_libraries\\(swbt_application[^\\)]*swbt_btstack"
                           "application target BTstack link boundary")
foreach(test_name
        application_control_lease_test
        application_command_test)
    swbt_assert_file_match("CMakeLists.txt"
                           "target_link_libraries\\(${test_name} PRIVATE swbt_application\\)"
                           "application test link boundary")
endforeach()

swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_switch_protocol STATIC"
                       "protocol target boundary")
foreach(test_name
        switch_report_test
        switch_hid_descriptor_test
        switch_subcommand_test
        switch_subcommand_reply_test
        switch_subcommand_dispatcher_test
        switch_spi_test
        switch_spi_seed_test
        switch_rumble_test
        switch_player_lights_test)
    swbt_assert_file_match("CMakeLists.txt"
                           "target_link_libraries\\(${test_name} PRIVATE swbt_switch_protocol\\)"
                           "protocol test link boundary")
endforeach()
