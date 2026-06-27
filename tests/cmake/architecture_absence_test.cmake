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
)

swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_support STATIC"
                       "support target")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_ipc STATIC"
                       "IPC target")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_btstack_bridge STATIC"
                       "BTstack bridge target")
swbt_assert_file_match("CMakeLists.txt" "add_library\\(swbt_daemon_process STATIC"
                       "daemon process target")
swbt_assert_file_match("CMakeLists.txt" "add_executable\\(architecture_journey_test"
                       "architecture journey test target")
swbt_assert_file_match("CMakeLists.txt"
                       "target_link_libraries\\(architecture_journey_test PRIVATE swbt_daemon_process\\)"
                       "architecture journey test link")

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
