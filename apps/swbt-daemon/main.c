#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#endif

#include "btstack_bridge/production_btstack.h"
#include "core/diagnostics.h"
#include "daemon/config.h"
#include "daemon/host.h"
#include "daemon/launch_options.h"
#include "daemon/production_backend.h"

#if defined(_WIN32)
static swbt_daemon_shutdown_request_t g_swbt_daemon_shutdown_request;
static void *g_swbt_daemon_shutdown_context;
static const char *g_swbt_daemon_crash_dump_path;
static volatile LONG g_swbt_daemon_crash_dump_written;

static void swbt_daemon_write_crash_dump(EXCEPTION_POINTERS *exception_info) {
    HANDLE file = INVALID_HANDLE_VALUE;
    MINIDUMP_EXCEPTION_INFORMATION dump_exception;

    if (!swbt_diagnostic_path_is_enabled(g_swbt_daemon_crash_dump_path)) {
        return;
    }
    if (InterlockedExchange(&g_swbt_daemon_crash_dump_written, 1) != 0) {
        return;
    }

    file = CreateFileA(g_swbt_daemon_crash_dump_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }

    dump_exception.ThreadId = GetCurrentThreadId();
    dump_exception.ExceptionPointers = exception_info;
    dump_exception.ClientPointers = FALSE;
    (void)MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal,
                            &dump_exception, NULL, NULL);
    CloseHandle(file);
}

static LONG WINAPI swbt_daemon_unhandled_exception_filter(EXCEPTION_POINTERS *exception_info) {
    swbt_daemon_write_crash_dump(exception_info);
    return EXCEPTION_EXECUTE_HANDLER;
}

static LONG WINAPI swbt_daemon_vectored_exception_handler(EXCEPTION_POINTERS *exception_info) {
    if (exception_info != NULL && exception_info->ExceptionRecord != NULL &&
        exception_info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        swbt_daemon_write_crash_dump(exception_info);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void swbt_daemon_install_crash_dump_handler(void) {
    g_swbt_daemon_crash_dump_path = getenv("SWBT_CRASH_DUMP_PATH");
    if (swbt_diagnostic_path_is_enabled(g_swbt_daemon_crash_dump_path)) {
        (void)AddVectoredExceptionHandler(1, swbt_daemon_vectored_exception_handler);
        SetUnhandledExceptionFilter(swbt_daemon_unhandled_exception_filter);
    }
}

static BOOL WINAPI swbt_daemon_console_control_handler(DWORD control_type) {
    switch (control_type) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        if (g_swbt_daemon_shutdown_request != NULL) {
            g_swbt_daemon_shutdown_request(g_swbt_daemon_shutdown_context);
        }
        return TRUE;
    default:
        return FALSE;
    }
}

static int swbt_daemon_install_process_shutdown_listener(
    void *context, swbt_daemon_shutdown_request_t request_shutdown, void *request_context) {
    (void)context;
    g_swbt_daemon_shutdown_request = request_shutdown;
    g_swbt_daemon_shutdown_context = request_context;
    return SetConsoleCtrlHandler(swbt_daemon_console_control_handler, TRUE) ? 0 : -1;
}

static void swbt_daemon_uninstall_process_shutdown_listener(void *context) {
    (void)context;
    (void)SetConsoleCtrlHandler(swbt_daemon_console_control_handler, FALSE);
    g_swbt_daemon_shutdown_request = NULL;
    g_swbt_daemon_shutdown_context = NULL;
}

static const swbt_daemon_shutdown_listener_t *swbt_daemon_process_shutdown_listener(void) {
    static const swbt_daemon_shutdown_listener_t listener = {
        .install = swbt_daemon_install_process_shutdown_listener,
        .uninstall = swbt_daemon_uninstall_process_shutdown_listener,
    };
    return &listener;
}
#else
static void swbt_daemon_install_crash_dump_handler(void) {}

static const swbt_daemon_shutdown_listener_t *swbt_daemon_process_shutdown_listener(void) {
    return NULL;
}
#endif

static swbt_daemon_config_env_t swbt_daemon_config_env_from_process_env(void) {
    return (swbt_daemon_config_env_t){
        .report_period_us = getenv("SWBT_REPORT_PERIOD_US"),
        .ipc_host = getenv("SWBT_IPC_HOST"),
        .ipc_port = getenv("SWBT_IPC_PORT"),
        .ipc_backlog = getenv("SWBT_IPC_BACKLOG"),
        .ipc_heartbeat_timeout_ms = getenv("SWBT_IPC_HEARTBEAT_TIMEOUT_MS"),
        .device_info_profile = getenv("SWBT_DEVICE_INFO_PROFILE"),
    };
}

static swbt_daemon_hardware_approval_t swbt_daemon_hardware_approval_from_process_env(void) {
    const swbt_daemon_hardware_approval_env_t env = {
        .run_hardware = getenv("SWBT_RUN_HARDWARE"),
        .hardware_approved = getenv("SWBT_HARDWARE_APPROVED"),
    };

    return swbt_daemon_hardware_approval_from_env(&env);
}

static int swbt_daemon_run_production(const swbt_daemon_launch_config_t *launch_config) {
    swbt_daemon_production_backend_t backend;
    const swbt_daemon_hardware_approval_t approval =
        swbt_daemon_hardware_approval_from_process_env();

    if (swbt_btstack_production_link_key_db_configure(
            launch_config->link_key_db_configured ? launch_config->link_key_db_path : NULL) != 0) {
        swbt_diagnostic_trace("production: link key db path invalid");
        return 1;
    }
    swbt_diagnostic_trace("production: backend init");
    if (swbt_daemon_production_backend_init(&backend, &launch_config->config,
                                            swbt_btstack_production_adapter(),
                                            NULL) != SWBT_DAEMON_PRODUCTION_OK) {
        swbt_diagnostic_trace("production: backend init failed");
        return 1;
    }
    if (launch_config->learned_switch_address_target_configured &&
        !swbt_daemon_production_backend_set_learned_switch_address_target(
            &backend, &launch_config->learned_switch_address_target)) {
        swbt_diagnostic_trace("production: learned switch address target invalid");
        return 1;
    }
    swbt_diagnostic_trace("production: enter main");
    return swbt_daemon_production_main_with_backend_and_shutdown(
               &backend, &approval, swbt_daemon_process_shutdown_listener(), NULL) ==
                   SWBT_DAEMON_PRODUCTION_OK
               ? 0
               : 1;
}

int main(int argc, char **argv) {
    swbt_daemon_launch_config_t launch_config;
    swbt_daemon_launch_options_t launch_options;
    const swbt_daemon_config_env_t config_env = swbt_daemon_config_env_from_process_env();

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    swbt_daemon_install_crash_dump_handler();
    swbt_diagnostic_trace("main: entered");
    if (swbt_daemon_launch_options_parse(&launch_options, argc, (const char *const *)argv) !=
        SWBT_DAEMON_LAUNCH_OPTIONS_OK) {
        swbt_diagnostic_trace("main: invalid CLI options");
        return 1;
    }
    if (!swbt_daemon_launch_config_prepare(&launch_config, &launch_options, &config_env)) {
        swbt_diagnostic_trace("main: invalid launch config");
        return 1;
    }
    switch (launch_options.backend) {
    case SWBT_DAEMON_LAUNCH_BACKEND_PRODUCTION:
        swbt_diagnostic_trace("main: selected production backend");
        return swbt_daemon_run_production(&launch_config);
    case SWBT_DAEMON_LAUNCH_BACKEND_NOOP:
        swbt_diagnostic_trace("main: selected noop backend");
        return swbt_daemon_main_with_host_backend(&launch_config.config,
                                                  swbt_daemon_host_noop_backend(), NULL);
    }
    swbt_diagnostic_trace("main: unknown backend");
    return 1;
}
