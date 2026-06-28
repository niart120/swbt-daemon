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
file(GLOB_RECURSE swbt_domain_files
    "${SWBT_SOURCE_DIR}/swbt/domain/*.c"
    "${SWBT_SOURCE_DIR}/swbt/domain/*.h"
)
foreach(path IN LISTS swbt_domain_files)
    file(RELATIVE_PATH relative_path "${SWBT_SOURCE_DIR}" "${path}")
    swbt_assert_file_not_match("${relative_path}" "#include \"(btstack_bridge|classic|daemon|ipc)/"
                               "domain boundary")
    swbt_assert_file_not_match("${relative_path}" "#include \"(bluetooth|btstack|gap|hci|l2cap)"
                               "domain BTstack header boundary")
endforeach()

file(GLOB_RECURSE swbt_protocol_files
    "${SWBT_SOURCE_DIR}/swbt/switch/*.c"
    "${SWBT_SOURCE_DIR}/swbt/switch/*.h"
)
foreach(path IN LISTS swbt_protocol_files)
    file(RELATIVE_PATH relative_path "${SWBT_SOURCE_DIR}" "${path}")
    swbt_assert_file_not_match("${relative_path}" "#include \"(domain|btstack_bridge|support|daemon|ipc)/"
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
                               "#include \"(ipc/|daemon/(process|ipc_runner)\\.h)"
                               "BTstack bridge IPC internal include boundary")
    swbt_assert_file_not_match("${relative_path}"
                               "swbt_daemon_ipc_runner"
                               "BTstack bridge IPC runner type boundary")
endforeach()

swbt_assert_file_not_match("swbt/btstack_bridge/production_btstack_impl.c"
                           "#include \"daemon/ipc_runner.h\""
                           "BTstack production adapter IPC runner include boundary")
swbt_assert_file_not_match("swbt/btstack_bridge/production_btstack_impl.h"
                           "daemon/production_runner.h"
                           "BTstack production implementation daemon runner struct boundary")
swbt_assert_file_not_match("CMakeLists.txt"
                           "add_library\\(swbt_daemon_process STATIC[^\\)]*production_"
                           "daemon process target production source boundary")

file(GLOB_RECURSE swbt_runtime_files
    "${SWBT_SOURCE_DIR}/swbt/runtime/*.c"
    "${SWBT_SOURCE_DIR}/swbt/runtime/*.h"
)
foreach(path IN LISTS swbt_runtime_files)
    file(RELATIVE_PATH relative_path "${SWBT_SOURCE_DIR}" "${path}")
    swbt_assert_file_not_match("${relative_path}" "#include \"(daemon|ipc)/"
                               "runtime daemon/IPC include boundary")
    swbt_assert_file_not_match("${relative_path}" "ipc_start"
                               "runtime backend IPC start contract boundary")
    swbt_assert_file_not_match("${relative_path}" "ipc_stop"
                               "runtime backend IPC stop contract boundary")
endforeach()

file(GLOB_RECURSE swbt_control_files
    "${SWBT_SOURCE_DIR}/swbt/control/*.c"
    "${SWBT_SOURCE_DIR}/swbt/control/*.h"
)
foreach(path IN LISTS swbt_control_files)
    file(RELATIVE_PATH relative_path "${SWBT_SOURCE_DIR}" "${path}")
    swbt_assert_file_not_match("${relative_path}" "#include \"(daemon|ipc)/"
                               "control daemon/IPC include boundary")
    swbt_assert_file_not_match("${relative_path}" "#include \"runtime/"
                               "control runtime include boundary")
endforeach()

# These checks cover CMake topology. Public include-root compile probes cover include visibility,
# but they do not prove the intended target names and unit-test link targets remain in place.
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_domain STATIC"
                       "domain target boundary")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_runtime STATIC"
                       "runtime target boundary")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_control STATIC"
                       "control target boundary")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_ipc STATIC"
                       "IPC target boundary")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_btstack_bridge STATIC"
                       "BTstack bridge target boundary")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_daemon_process STATIC"
                       "daemon process target boundary")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_daemon_production_runner STATIC"
                       "daemon production runner target boundary")
swbt_assert_file_not_match("CMakeLists.txt"
                           "target_link_libraries\\(swbt_runtime[^\\)]*swbt_ipc"
                           "runtime target IPC link boundary")
swbt_assert_file_not_match("CMakeLists.txt"
                           "target_link_libraries\\(swbt_control[^\\)]*swbt_ipc"
                           "control target IPC link boundary")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt SHARED"
                       "public C ABI target boundary")
swbt_assert_file_not_match("CMakeLists.txt"
                           "target_link_libraries\\(swbt[ \r\n][^\\)]*swbt_ipc"
                           "public C ABI IPC link boundary")
swbt_assert_file_not_match("CMakeLists.txt"
                           "target_link_libraries\\(swbt_domain[^\\)]*swbt_btstack"
                           "domain target BTstack link boundary")
foreach(test_name
        domain_lease_test
        domain_command_test)
    swbt_assert_file_match("CMakeLists.txt"
                           "target_link_libraries\\(${test_name} PRIVATE swbt_domain\\)"
                           "domain test link boundary")
endforeach()
swbt_assert_file_match("CMakeLists.txt"
                       "target_link_libraries\\(runtime_host_test PRIVATE swbt_runtime\\)"
                       "runtime test link boundary")
swbt_assert_file_match("CMakeLists.txt"
                       "target_link_libraries\\(control_test PRIVATE swbt_control\\)"
                       "control test link boundary")

foreach(test_name
        daemon_active_reconnect_test
        daemon_btstack_ipc_pump_adapter_test
        daemon_production_runner_test
        daemon_btstack_process_backend_test
        daemon_shutdown_sequence_test
        daemon_btstack_hid_session_test
        daemon_btstack_report_timer_bridge_test)
    swbt_assert_file_match("CMakeLists.txt"
                           "target_link_libraries\\(${test_name} PRIVATE swbt_daemon_production_runner\\)"
                           "daemon production test link boundary")
endforeach()
swbt_assert_file_match("CMakeLists.txt"
                       "target_link_libraries\\(daemon_production_hid_sdp_record_test PRIVATE[ \t\r\n]+swbt_daemon_production_runner"
                       "daemon production HID SDP test link boundary")

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
