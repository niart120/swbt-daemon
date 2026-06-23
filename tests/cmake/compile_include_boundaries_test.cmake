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

set(application_public_include_dir
    "${SWBT_BINARY_DIR}/swbt_public_includes/swbt_application"
)
set(support_public_include_dir
    "${SWBT_BINARY_DIR}/swbt_public_includes/swbt_support"
)
set(protocol_public_include_dir
    "${SWBT_BINARY_DIR}/swbt_public_includes/swbt_switch_protocol"
)
if(NOT IS_DIRECTORY "${application_public_include_dir}")
    message(FATAL_ERROR
        "swbt_application public include root is missing: ${application_public_include_dir}"
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

swbt_expect_compile_result(
    application_public_header_visible
    TRUE
    "#include \"application/app.h\"\nint main(void) { return 0; }\n"
    "${application_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
swbt_expect_compile_result(
    application_btstack_adapter_hidden
    FALSE
    "#include \"btstack_bridge/production_adapter.h\"\nint main(void) { return 0; }\n"
    "${application_public_include_dir}"
    "${support_public_include_dir}"
    "${protocol_public_include_dir}"
)
