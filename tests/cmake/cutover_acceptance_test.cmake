if(NOT DEFINED SWBT_SOURCE_DIR)
    message(FATAL_ERROR "SWBT_SOURCE_DIR is required")
endif()

set(cmake_lists_path "${SWBT_SOURCE_DIR}/CMakeLists.txt")
set(cutover_record_path
    "${SWBT_SOURCE_DIR}/work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md")
set(runtime_spec_path "${SWBT_SOURCE_DIR}/spec/architecture/daemon-runtime-boundaries.md")
set(rearchitecture_spec_path
    "${SWBT_SOURCE_DIR}/spec/architecture/daemon-application-boundary-rearchitecture.md")

file(READ "${cmake_lists_path}" cmake_lists)
file(READ "${cutover_record_path}" cutover_record)
file(READ "${runtime_spec_path}" runtime_spec)
file(READ "${rearchitecture_spec_path}" rearchitecture_spec)

if(NOT cmake_lists MATCHES "add_executable\\(daemon_cutover_journey_test")
    message(FATAL_ERROR "daemon cutover synthetic journey test target is required")
endif()

if(NOT cmake_lists MATCHES "target_link_libraries\\(daemon_cutover_journey_test PRIVATE swbt_core\\)")
    message(FATAL_ERROR "daemon cutover synthetic journey test must link swbt_core")
endif()

foreach(name
        "production_btstack[^\\n]*IPC pump"
        "production backend ops table"
        "swbt_ipc_session_t"
        "state_mailbox"
        "swbt_core[^\\n]*aggregate target")
    if(NOT cutover_record MATCHES "${name}[^\\n]*\\|[^\\n]*(kept|removed|deferred)")
        message(FATAL_ERROR "cutover inventory must classify ${name}")
    endif()
endforeach()

if(cutover_record MATCHES "\\|[^\\n]*unclassified[^\\n]*\\|")
    message(FATAL_ERROR "cutover inventory must not contain unclassified items")
endif()

foreach(name
        "production_btstack[^\\n]*IPC pump"
        "production backend ops table"
        "swbt_ipc_session_t"
        "state_mailbox"
        "swbt_core[^\\n]*aggregate target")
    if(NOT runtime_spec MATCHES "${name}")
        message(FATAL_ERROR "runtime boundary spec must define current responsibility for ${name}")
    endif()
endforeach()

if(NOT rearchitecture_spec MATCHES "local_055.*cutover acceptance")
    message(FATAL_ERROR "rearchitecture spec must record local_055 cutover acceptance")
endif()
