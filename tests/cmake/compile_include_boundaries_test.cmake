if(NOT DEFINED SWBT_BINARY_DIR)
    message(FATAL_ERROR "SWBT_BINARY_DIR is required")
endif()
if(NOT DEFINED SWBT_C_COMPILER)
    message(FATAL_ERROR "SWBT_C_COMPILER is required")
endif()

function(swbt_expect_compile_result name expected_result source)
    set(probe_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}")
    file(MAKE_DIRECTORY "${probe_dir}")
    file(WRITE "${probe_dir}/${name}.c" "${source}")
    set(include_flags)
    foreach(include_dir IN LISTS ARGN)
        list(APPEND include_flags "-I${include_dir}")
    endforeach()
    execute_process(
        COMMAND
            "${SWBT_C_COMPILER}"
            -std=c11
            ${include_flags}
            -c
            "${probe_dir}/${name}.c"
            -o
            "${probe_dir}/${name}.o"
        RESULT_VARIABLE compile_status
        OUTPUT_VARIABLE compile_stdout
        ERROR_VARIABLE compile_stderr
    )
    set(compile_result FALSE)
    if(compile_status EQUAL 0)
        set(compile_result TRUE)
    endif()
    if(NOT compile_result STREQUAL expected_result)
        message(FATAL_ERROR
            "${name}: expected compile result ${expected_result}, got ${compile_result}\n"
            "${compile_stdout}${compile_stderr}"
        )
    endif()
endfunction()

set(domain_public_include_dir
    "${SWBT_BINARY_DIR}/swbt_public_includes/swbt_domain"
)
set(support_public_include_dir
    "${SWBT_BINARY_DIR}/swbt_public_includes/swbt_support"
)
set(protocol_public_include_dir
    "${SWBT_BINARY_DIR}/swbt_public_includes/swbt_switch_protocol"
)
set(btstack_bridge_public_include_dir
    "${SWBT_BINARY_DIR}/swbt_public_includes/swbt_btstack_bridge"
)
set(runtime_public_include_dir
    "${SWBT_BINARY_DIR}/swbt_public_includes/swbt_runtime"
)
set(control_public_include_dir
    "${SWBT_BINARY_DIR}/swbt_public_includes/swbt_control"
)
set(ipc_public_include_dir
    "${SWBT_BINARY_DIR}/swbt_public_includes/swbt_ipc"
)
set(daemon_process_public_include_dir
    "${SWBT_BINARY_DIR}/swbt_public_includes/swbt_daemon_process"
)
set(daemon_production_runner_public_include_dir
    "${SWBT_BINARY_DIR}/swbt_public_includes/swbt_daemon_production_runner"
)
if(NOT IS_DIRECTORY "${domain_public_include_dir}")
    message(FATAL_ERROR
        "swbt_domain public include root is missing: ${domain_public_include_dir}"
    )
endif()
if(NOT IS_DIRECTORY "${support_public_include_dir}")
    message(FATAL_ERROR
        "swbt_support public include root is missing: ${support_public_include_dir}"
    )
endif()
if(NOT IS_DIRECTORY "${protocol_public_include_dir}")
    message(FATAL_ERROR
        "swbt_switch_protocol public include root is missing: ${protocol_public_include_dir}"
    )
endif()
if(NOT IS_DIRECTORY "${btstack_bridge_public_include_dir}")
    message(FATAL_ERROR
        "swbt_btstack_bridge public include root is missing: "
        "${btstack_bridge_public_include_dir}"
    )
endif()
if(NOT IS_DIRECTORY "${runtime_public_include_dir}")
    message(FATAL_ERROR
        "swbt_runtime public include root is missing: ${runtime_public_include_dir}"
    )
endif()
if(NOT IS_DIRECTORY "${control_public_include_dir}")
    message(FATAL_ERROR
        "swbt_control public include root is missing: ${control_public_include_dir}"
    )
endif()
if(NOT IS_DIRECTORY "${daemon_process_public_include_dir}")
    message(FATAL_ERROR
        "swbt_daemon_process public include root is missing: ${daemon_process_public_include_dir}"
    )
endif()
if(NOT IS_DIRECTORY "${daemon_production_runner_public_include_dir}")
    message(FATAL_ERROR
        "swbt_daemon_production_runner public include root is missing: "
        "${daemon_production_runner_public_include_dir}"
    )
endif()
if(NOT IS_DIRECTORY "${ipc_public_include_dir}")
    message(FATAL_ERROR
        "swbt_ipc public include root is missing: ${ipc_public_include_dir}"
    )
endif()

swbt_expect_compile_result(
    domain_public_header_visible
    TRUE
    "#include \"domain/domain.h\"\nint main(void) { return 0; }\n"
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    domain_btstack_bridge_hidden
    FALSE
    "#include \"btstack_bridge/production_ports.h\"\nint main(void) { return 0; }\n"
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    btstack_bridge_public_header_visible
    TRUE
    "#include \"btstack_bridge/hid_device_registration.h\"\nint main(void) { return 0; }\n"
    "${btstack_bridge_public_include_dir}"
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    btstack_bridge_ipc_transport_hidden
    FALSE
    "#include \"ipc/ipc_adapter.h\"\nint main(void) { return 0; }\n"
    "${btstack_bridge_public_include_dir}"
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    runtime_public_header_visible
    TRUE
    "#include \"runtime/host.h\"\nint main(void) { return 0; }\n"
    "${runtime_public_include_dir}"
    "${control_public_include_dir}"
    "${btstack_bridge_public_include_dir}"
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    runtime_ipc_transport_hidden
    FALSE
    "#include \"ipc/ipc_adapter.h\"\nint main(void) { return 0; }\n"
    "${runtime_public_include_dir}"
    "${control_public_include_dir}"
    "${btstack_bridge_public_include_dir}"
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    control_public_header_visible
    TRUE
    "#include \"control/control.h\"\nint main(void) { return 0; }\n"
    "${control_public_include_dir}"
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    control_runtime_host_hidden
    FALSE
    "#include \"runtime/host.h\"\nint main(void) { return 0; }\n"
    "${control_public_include_dir}"
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    control_ipc_transport_hidden
    FALSE
    "#include \"ipc/ipc_adapter.h\"\nint main(void) { return 0; }\n"
    "${control_public_include_dir}"
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    daemon_process_public_header_visible
    TRUE
    "#include \"daemon/process.h\"\nint main(void) { return 0; }\n"
    "${daemon_process_public_include_dir}"
    "${ipc_public_include_dir}"
    "${control_public_include_dir}"
    "${runtime_public_include_dir}"
    "${btstack_bridge_public_include_dir}"
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    daemon_ipc_runner_header_visible
    TRUE
    "#include \"daemon/ipc_runner.h\"\nint main(void) { return 0; }\n"
    "${daemon_process_public_include_dir}"
    "${ipc_public_include_dir}"
    "${control_public_include_dir}"
    "${runtime_public_include_dir}"
    "${btstack_bridge_public_include_dir}"
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    daemon_process_production_runner_hidden
    FALSE
    "#include \"daemon/production_runner.h\"\nint main(void) { return 0; }\n"
    "${daemon_process_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    daemon_process_btstack_ipc_pump_adapter_hidden
    FALSE
    "#include \"daemon/btstack_ipc_pump_adapter.h\"\nint main(void) { return 0; }\n"
    "${daemon_process_public_include_dir}"
    "${ipc_public_include_dir}"
    "${control_public_include_dir}"
    "${runtime_public_include_dir}"
    "${btstack_bridge_public_include_dir}"
    ${SWBT_BTSTACK_INCLUDE_DIRS}
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    production_runner_public_header_visible
    TRUE
    "#include \"daemon/production_runner.h\"\nint main(void) { return 0; }\n"
    "${daemon_production_runner_public_include_dir}"
    "${daemon_process_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    production_btstack_ipc_pump_adapter_header_visible
    TRUE
    "#include \"daemon/btstack_ipc_pump_adapter.h\"\nint main(void) { return 0; }\n"
    "${daemon_production_runner_public_include_dir}"
    "${daemon_process_public_include_dir}"
    "${ipc_public_include_dir}"
    "${control_public_include_dir}"
    "${runtime_public_include_dir}"
    "${btstack_bridge_public_include_dir}"
    ${SWBT_BTSTACK_INCLUDE_DIRS}
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    production_shutdown_listener_header_visible
    TRUE
    "#include \"daemon/shutdown_listener.h\"\nint main(void) { return 0; }\n"
    "${daemon_production_runner_public_include_dir}"
)
swbt_expect_compile_result(
    production_runner_internal_header_hidden
    FALSE
    "#include \"daemon/production_runner_internal.h\"\nint main(void) { return 0; }\n"
    "${daemon_production_runner_public_include_dir}"
    "${daemon_process_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    btstack_bridge_daemon_process_hidden
    FALSE
    "#include \"daemon/process.h\"\nint main(void) { return 0; }\n"
    "${btstack_bridge_public_include_dir}"
    "${domain_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
