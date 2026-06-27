if(NOT DEFINED SWBT_SOURCE_DIR)
    message(FATAL_ERROR "SWBT_SOURCE_DIR is required")
endif()

function(swbt_assert_file_exists relative_path label)
    set(path "${SWBT_SOURCE_DIR}/${relative_path}")
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "${label}: ${relative_path} is missing")
    endif()
endfunction()

function(swbt_assert_file_match relative_path pattern label)
    set(path "${SWBT_SOURCE_DIR}/${relative_path}")
    file(READ "${path}" content)
    if(NOT content MATCHES "${pattern}")
        message(FATAL_ERROR "${label}: ${relative_path} did not match required pattern ${pattern}")
    endif()
endfunction()

function(swbt_assert_file_not_match relative_path pattern label)
    set(path "${SWBT_SOURCE_DIR}/${relative_path}")
    file(READ "${path}" content)
    if(content MATCHES "${pattern}")
        message(FATAL_ERROR "${label}: ${relative_path} matched forbidden pattern ${pattern}")
    endif()
endfunction()

swbt_assert_file_exists("apps/swbt-daemon/production_entrypoint.c"
                        "production entrypoint source")
swbt_assert_file_exists("apps/swbt-daemon/production_entrypoint.h"
                        "production entrypoint header")
swbt_assert_file_exists("apps/swbt-daemon/platform_process.c"
                        "platform process source")
swbt_assert_file_exists("apps/swbt-daemon/platform_process.h"
                        "platform process header")

swbt_assert_file_not_match("apps/swbt-daemon/main.c"
                           "production_btstack_impl"
                           "main production BTstack wiring boundary")
swbt_assert_file_not_match("apps/swbt-daemon/main.c"
                           "production_runner"
                           "main production runner boundary")
swbt_assert_file_not_match("apps/swbt-daemon/main.c"
                           "windows\\.h|dbghelp\\.h|MiniDumpWriteDump|SetConsoleCtrlHandler"
                           "main Windows process support boundary")

swbt_assert_file_match("apps/swbt-daemon/main.c"
                       "swbt_daemon_production_entrypoint_run"
                       "main production entrypoint dispatch")
swbt_assert_file_match("apps/swbt-daemon/production_entrypoint.c"
                       "swbt_btstack_production_link_key_db_configure"
                       "production entrypoint link key DB setup")
swbt_assert_file_match("apps/swbt-daemon/production_entrypoint.c"
                       "swbt_daemon_production_main_with_runner_and_shutdown"
                       "production entrypoint runner handoff")
swbt_assert_file_match("apps/swbt-daemon/production_entrypoint.c"
                       "swbt_daemon_platform_shutdown_listener"
                       "production entrypoint process shutdown listener")
swbt_assert_file_match("apps/swbt-daemon/platform_process.c"
                       "swbt_daemon_platform_install_crash_dump_handler"
                       "platform crash dump entrypoint")
swbt_assert_file_match("apps/swbt-daemon/platform_process.c"
                       "swbt_daemon_platform_shutdown_listener"
                       "platform shutdown listener entrypoint")

swbt_assert_file_match("CMakeLists.txt"
                       "apps/swbt-daemon/production_entrypoint\\.c"
                       "swbt-daemon production entrypoint source list")
swbt_assert_file_match("CMakeLists.txt"
                       "apps/swbt-daemon/platform_process\\.c"
                       "swbt-daemon platform process source list")
