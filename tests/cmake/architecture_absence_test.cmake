if(NOT DEFINED SWBT_SOURCE_DIR)
    message(FATAL_ERROR "SWBT_SOURCE_DIR is required")
endif()

function(swbt_assert_file_match relative_path pattern label)
    set(path "${SWBT_SOURCE_DIR}/${relative_path}")
    file(READ "${path}" content)
    if(NOT content MATCHES "${pattern}")
        message(FATAL_ERROR "${label}: ${relative_path} did not match required pattern")
    endif()
endfunction()

function(swbt_assert_file_not_match relative_path pattern label)
    set(path "${SWBT_SOURCE_DIR}/${relative_path}")
    file(READ "${path}" content)
    if(content MATCHES "${pattern}")
        message(FATAL_ERROR "${label}: ${relative_path} matched forbidden pattern")
    endif()
endfunction()

function(swbt_add_token out_var prefix suffix)
    set(token "${prefix}")
    string(APPEND token "${suffix}")
    set(${out_var} "${token}" PARENT_SCOPE)
endfunction()

swbt_add_token(old_ipc_type "swbt_ipc_" "session_t")
swbt_add_token(old_ipc_file "ipc_" "session")
swbt_add_token(old_mailbox "state_" "mailbox")
swbt_add_token(old_host_predecessor "swbt_daemon_" "runtime")
swbt_add_token(old_host_predecessor_macro "SWBT_DAEMON_" "RUNTIME")
swbt_add_token(old_backend_contract "production_backend_" "ops")
swbt_add_token(old_core_target "swbt_" "core")
swbt_add_token(old_journey_name "daemon_" "cutover")
swbt_add_token(old_acceptance_name "cutover_" "acceptance")
swbt_add_token(old_daemon_process_runtime_wrapper "swbt_daemon_process_" "runtime_")
swbt_add_token(control_runtime_include "#include \"runtime/" "host.h\"")

set(forbidden_tokens
    "${old_ipc_type}"
    "${old_ipc_file}"
    "${old_mailbox}"
    "${old_host_predecessor}"
    "${old_host_predecessor_macro}"
    "${old_backend_contract}"
    "${old_core_target}"
    "${old_journey_name}"
    "${old_acceptance_name}"
    "${old_daemon_process_runtime_wrapper}"
)

set(control_forbidden_tokens
    "${control_runtime_include}"
)

swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_support STATIC"
                       "support target")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_ipc STATIC"
                       "IPC target")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_btstack_bridge STATIC"
                       "BTstack bridge target")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_daemon_process STATIC"
                       "daemon process target")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_daemon_production_runner STATIC"
                       "daemon production runner target")
swbt_assert_file_match("CMakeLists.txt" "add_executable\\(architecture_journey_test"
                       "architecture journey test target")
swbt_assert_file_match("CMakeLists.txt"
                       "target_link_libraries\\(architecture_journey_test[^\\)]*swbt_daemon_process"
                       "architecture journey test link")
swbt_assert_file_not_match("swbt/daemon/process.h"
                           "swbt_domain_t[ \t\r\n]*[*][ \t\r\n]*app"
                           "daemon process app ownership boundary")
swbt_assert_file_not_match("swbt/daemon/process.h"
                           "swbt_control_t[ \t\r\n]+control"
                           "daemon process control ownership boundary")
swbt_assert_file_not_match("swbt/daemon/process.h"
                           "swbt_runtime_host_backend_t[ \t\r\n]+runtime_backend"
                           "daemon process runtime backend ownership boundary")
swbt_assert_file_not_match("swbt/daemon/process.h"
                           "(const[ \t\r\n]+)?swbt_runtime_host_backend_t[ \t\r\n]*[*][ \t\r\n]*runtime_backend"
                           "daemon process runtime backend pointer boundary")
swbt_assert_file_not_match("swbt/daemon/process.h"
                           "#include \"runtime/host\\.h\""
                           "daemon process runtime host public include boundary")
swbt_assert_file_not_match("swbt/daemon/process.h"
                           "#include \"btstack_bridge/output_report_handler\\.h\""
                           "daemon process output handler public include boundary")
foreach(callback_name
        hid_register
        hid_stop
        output_handler_start
        output_handler_stop
        report_timer_start
        report_timer_stop
        report_timer_send_neutral_now
        subcommand_reply_enqueue
        read_device_info
        time_ms)
    swbt_assert_file_not_match("swbt/daemon/process.h"
                               "${callback_name}"
                               "daemon process runtime callback boundary")
endforeach()

set(files_to_scan "${SWBT_SOURCE_DIR}/CMakeLists.txt")
foreach(root apps api swbt tests)
    file(GLOB_RECURSE files
        "${SWBT_SOURCE_DIR}/${root}/*.c"
        "${SWBT_SOURCE_DIR}/${root}/*.h"
    )
    list(APPEND files_to_scan ${files})
endforeach()

foreach(path IN LISTS files_to_scan)
    if(path MATCHES "/tests/cmake/")
        continue()
    endif()
    file(READ "${path}" content)
    foreach(token IN LISTS forbidden_tokens)
        if(content MATCHES "${token}")
            file(RELATIVE_PATH relative_path "${SWBT_SOURCE_DIR}" "${path}")
            message(FATAL_ERROR "${relative_path} still contains removed architecture token")
        endif()
    endforeach()
endforeach()

file(GLOB_RECURSE control_files
    "${SWBT_SOURCE_DIR}/swbt/control/*.c"
    "${SWBT_SOURCE_DIR}/swbt/control/*.h"
)
foreach(path IN LISTS control_files)
    file(READ "${path}" content)
    foreach(token IN LISTS control_forbidden_tokens)
        if(content MATCHES "${token}")
            file(RELATIVE_PATH relative_path "${SWBT_SOURCE_DIR}" "${path}")
            message(FATAL_ERROR "${relative_path} still depends on runtime host concrete header")
        endif()
    endforeach()
endforeach()
