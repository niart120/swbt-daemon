#include "platform_process.h"

#include <stddef.h>

#include "support/diagnostics.h"

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>

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

void swbt_daemon_platform_install_crash_dump_handler(const char *path) {
    g_swbt_daemon_crash_dump_path = path;
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

const swbt_daemon_shutdown_listener_t *swbt_daemon_platform_shutdown_listener(void) {
    static const swbt_daemon_shutdown_listener_t listener = {
        .install = swbt_daemon_install_process_shutdown_listener,
        .uninstall = swbt_daemon_uninstall_process_shutdown_listener,
    };
    return &listener;
}
#else
void swbt_daemon_platform_install_crash_dump_handler(const char *path) {
    (void)path;
}

const swbt_daemon_shutdown_listener_t *swbt_daemon_platform_shutdown_listener(void) {
    return NULL;
}
#endif
