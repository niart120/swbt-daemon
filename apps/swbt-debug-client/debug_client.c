#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
// NOLINTNEXTLINE(bugprone-reserved-identifier): POSIX feature test macro.
#define _POSIX_C_SOURCE 199309L
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <time.h>
#endif

#include "debug_client.h"

// Keep this mask in sync with daemon-ipc-v1 button names.
static const uint32_t SWBT_DEBUG_CLIENT_DEFINED_BUTTON_MASK =
    SWBT_BUTTON_Y | SWBT_BUTTON_X | SWBT_BUTTON_B | SWBT_BUTTON_A | SWBT_BUTTON_SR_R |
    SWBT_BUTTON_SL_R | SWBT_BUTTON_R | SWBT_BUTTON_ZR | SWBT_BUTTON_MINUS | SWBT_BUTTON_PLUS |
    SWBT_BUTTON_R_STICK | SWBT_BUTTON_L_STICK | SWBT_BUTTON_HOME | SWBT_BUTTON_CAPTURE |
    SWBT_BUTTON_DOWN | SWBT_BUTTON_UP | SWBT_BUTTON_RIGHT | SWBT_BUTTON_LEFT | SWBT_BUTTON_SR_L |
    SWBT_BUTTON_SL_L | SWBT_BUTTON_L | SWBT_BUTTON_ZL;

typedef struct {
    const char *name;
    uint32_t mask;
} swbt_debug_client_button_name_t;

static const swbt_debug_client_button_name_t SWBT_DEBUG_CLIENT_BUTTONS[] = {
    {"y", SWBT_BUTTON_Y},
    {"x", SWBT_BUTTON_X},
    {"b", SWBT_BUTTON_B},
    {"a", SWBT_BUTTON_A},
    {"sr-r", SWBT_BUTTON_SR_R},
    {"sl-r", SWBT_BUTTON_SL_R},
    {"r", SWBT_BUTTON_R},
    {"zr", SWBT_BUTTON_ZR},
    {"minus", SWBT_BUTTON_MINUS},
    {"plus", SWBT_BUTTON_PLUS},
    {"r-stick", SWBT_BUTTON_R_STICK},
    {"l-stick", SWBT_BUTTON_L_STICK},
    {"home", SWBT_BUTTON_HOME},
    {"capture", SWBT_BUTTON_CAPTURE},
    {"down", SWBT_BUTTON_DOWN},
    {"up", SWBT_BUTTON_UP},
    {"right", SWBT_BUTTON_RIGHT},
    {"left", SWBT_BUTTON_LEFT},
    {"sr-l", SWBT_BUTTON_SR_L},
    {"sl-l", SWBT_BUTTON_SL_L},
    {"l", SWBT_BUTTON_L},
    {"zl", SWBT_BUTTON_ZL},
};

static void swbt_debug_client_write_message(FILE *err, const char *message) {
    fputs(message, err);
}

static void swbt_debug_client_write_option_message(FILE *err, const char *option,
                                                   const char *message) {
    fputs(option, err);
    fputs(message, err);
}

static void swbt_debug_client_write_u16(FILE *err, uint16_t value) {
    char digits[5];
    size_t digit_count = 0;
    unsigned int remaining = value;

    do {
        digits[digit_count] = (char)('0' + (remaining % 10u));
        ++digit_count;
        remaining /= 10u;
    } while (remaining != 0u);

    while (digit_count > 0u) {
        --digit_count;
        fputc(digits[digit_count], err);
    }
}

static void swbt_debug_client_sleep_ms(uint32_t milliseconds) {
    if (milliseconds == 0u) {
        return;
    }
#if defined(_WIN32)
    Sleep(milliseconds);
#else
    struct timespec remaining = {
        .tv_sec = (time_t)(milliseconds / 1000u),
        .tv_nsec = (long)((milliseconds % 1000u) * 1000000u),
    };
    while (remaining.tv_sec != 0 || remaining.tv_nsec != 0) {
        if (nanosleep(&remaining, &remaining) == 0) {
            break;
        }
        if (errno != EINTR) {
            break;
        }
    }
#endif
}

static bool swbt_debug_client_is_unsupported_macro_arg(const char *arg) {
    return strcmp(arg, "--tap") == 0 || strcmp(arg, "--duration-ms") == 0 ||
           strcmp(arg, "--sequence") == 0 || strcmp(arg, "--at-ms") == 0;
}

static int swbt_debug_client_parse_u64(const char *text, uint64_t max_value, uint64_t *out) {
    char *end = NULL;
    unsigned long long parsed = 0;

    if (text == NULL || text[0] == '\0' || text[0] == '-') {
        return -1;
    }

    errno = 0;
    parsed = strtoull(text, &end, 0);
    if (text == end || errno != 0 || *end != '\0' || parsed > max_value) {
        return -1;
    }

    *out = (uint64_t)parsed;
    return 0;
}

static int swbt_debug_client_parse_i64(const char *text, int64_t min_value, int64_t max_value,
                                       int64_t *out) {
    char *end = NULL;
    long long parsed = 0;

    if (text == NULL || text[0] == '\0') {
        return -1;
    }

    errno = 0;
    parsed = strtoll(text, &end, 0);
    if (text == end || errno != 0 || *end != '\0' || parsed < min_value || parsed > max_value) {
        return -1;
    }

    *out = (int64_t)parsed;
    return 0;
}

static int swbt_debug_client_require_value(int argc, const char *const *argv, int index,
                                           FILE *err) {
    if (index + 1 >= argc) {
        swbt_debug_client_write_option_message(err, argv[index], " requires a value\n");
        return -1;
    }
    return 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static int swbt_debug_client_parse_stick(const char *option, const char *text, uint16_t *out,
                                         FILE *err) {
    uint64_t parsed = 0;
    if (swbt_debug_client_parse_u64(text, 4095u, &parsed) != 0) {
        swbt_debug_client_write_option_message(err, option, " must be in range 0..4095\n");
        return -1;
    }
    *out = (uint16_t)parsed;
    return 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static int swbt_debug_client_parse_i16_option(const char *option, const char *text, int16_t *out,
                                              FILE *err) {
    int64_t parsed = 0;
    if (swbt_debug_client_parse_i64(text, INT16_MIN, INT16_MAX, &parsed) != 0) {
        swbt_debug_client_write_option_message(err, option, " must be in range -32768..32767\n");
        return -1;
    }
    *out = (int16_t)parsed;
    return 0;
}

static int swbt_debug_client_parse_button_name(const char *text, uint32_t *out) {
    for (size_t index = 0;
         index < sizeof(SWBT_DEBUG_CLIENT_BUTTONS) / sizeof(SWBT_DEBUG_CLIENT_BUTTONS[0]);
         ++index) {
        if (strcmp(text, SWBT_DEBUG_CLIENT_BUTTONS[index].name) == 0) {
            *out = SWBT_DEBUG_CLIENT_BUTTONS[index].mask;
            return 0;
        }
    }
    return -1;
}

static int swbt_debug_client_parse_args(int argc, const char *const *argv,
                                        swbt_debug_client_config_t *out_config, FILE *err) {
    swbt_debug_client_config_t config = {
        .port = 0,
        .state = {0},
    };
    config.state = swbt_state_neutral();

    if (argc <= 1) {
        swbt_debug_client_write_message(
            err, "usage: swbt-debug-client --port <port> [state options] [--hold-ms <ms>] "
                 "[--skip-release]\n");
        return -1;
    }

    for (int index = 1; index < argc; ++index) {
        const char *arg = argv[index];
        if (swbt_debug_client_is_unsupported_macro_arg(arg)) {
            swbt_debug_client_write_option_message(
                err, arg, " is not supported by the daemon IPC debug client\n");
            return -1;
        }
        if (strcmp(arg, "--port") == 0) {
            uint64_t parsed = 0;
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_u64(argv[index + 1], UINT16_MAX, &parsed) != 0 ||
                parsed == 0u) {
                swbt_debug_client_write_message(err, "--port must be in range 1..65535\n");
                return -1;
            }
            config.port = (uint16_t)parsed;
            ++index;
        } else if (strcmp(arg, "--buttons") == 0) {
            uint64_t parsed = 0;
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_u64(argv[index + 1], UINT32_MAX, &parsed) != 0 ||
                (((uint32_t)parsed) & ~SWBT_DEBUG_CLIENT_DEFINED_BUTTON_MASK) != 0u) {
                swbt_debug_client_write_message(err,
                                                "--buttons contains an undefined button bit\n");
                return -1;
            }
            config.state.buttons = (uint32_t)parsed;
            ++index;
        } else if (strcmp(arg, "--button") == 0) {
            uint32_t mask = 0;
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_button_name(argv[index + 1], &mask) != 0) {
                swbt_debug_client_write_message(err,
                                                "--button must be a daemon-ipc-v1 button name\n");
                return -1;
            }
            config.state.buttons |= mask;
            ++index;
        } else if (strcmp(arg, "--lx") == 0) {
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_stick(arg, argv[index + 1], &config.state.lx, err) != 0) {
                return -1;
            }
            ++index;
        } else if (strcmp(arg, "--ly") == 0) {
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_stick(arg, argv[index + 1], &config.state.ly, err) != 0) {
                return -1;
            }
            ++index;
        } else if (strcmp(arg, "--rx") == 0) {
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_stick(arg, argv[index + 1], &config.state.rx, err) != 0) {
                return -1;
            }
            ++index;
        } else if (strcmp(arg, "--ry") == 0) {
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_stick(arg, argv[index + 1], &config.state.ry, err) != 0) {
                return -1;
            }
            ++index;
        } else if (strcmp(arg, "--accel-x") == 0) {
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_i16_option(arg, argv[index + 1], &config.state.accel_x,
                                                   err) != 0) {
                return -1;
            }
            ++index;
        } else if (strcmp(arg, "--accel-y") == 0) {
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_i16_option(arg, argv[index + 1], &config.state.accel_y,
                                                   err) != 0) {
                return -1;
            }
            ++index;
        } else if (strcmp(arg, "--accel-z") == 0) {
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_i16_option(arg, argv[index + 1], &config.state.accel_z,
                                                   err) != 0) {
                return -1;
            }
            ++index;
        } else if (strcmp(arg, "--gyro-x") == 0) {
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_i16_option(arg, argv[index + 1], &config.state.gyro_x,
                                                   err) != 0) {
                return -1;
            }
            ++index;
        } else if (strcmp(arg, "--gyro-y") == 0) {
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_i16_option(arg, argv[index + 1], &config.state.gyro_y,
                                                   err) != 0) {
                return -1;
            }
            ++index;
        } else if (strcmp(arg, "--gyro-z") == 0) {
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_i16_option(arg, argv[index + 1], &config.state.gyro_z,
                                                   err) != 0) {
                return -1;
            }
            ++index;
        } else if (strcmp(arg, "--seq") == 0) {
            uint64_t parsed = 0;
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_u64(argv[index + 1], UINT64_MAX, &parsed) != 0) {
                swbt_debug_client_write_message(err, "--seq must be a non-negative integer\n");
                return -1;
            }
            config.state.client_seq = parsed;
            ++index;
        } else if (strcmp(arg, "--hold-ms") == 0) {
            uint64_t parsed = 0;
            if (swbt_debug_client_require_value(argc, argv, index, err) != 0 ||
                swbt_debug_client_parse_u64(argv[index + 1], UINT32_MAX, &parsed) != 0) {
                swbt_debug_client_write_message(err, "--hold-ms must be a non-negative integer\n");
                return -1;
            }
            config.hold_ms = (uint32_t)parsed;
            ++index;
        } else if (strcmp(arg, "--skip-release") == 0) {
            config.skip_release = true;
        } else if (strcmp(arg, "--neutral") == 0) {
            config.state = swbt_state_neutral();
        } else {
            swbt_debug_client_write_option_message(err, "unknown option: ", arg);
            swbt_debug_client_write_message(err, "\n");
            return -1;
        }
    }

    if (config.port == 0u) {
        swbt_debug_client_write_message(err, "--port is required\n");
        return -1;
    }

    *out_config = config;
    return 0;
}

int swbt_debug_client_send_hello(swbt_ipc_socket_t *socket) {
    const char request[] = "{\"v\":1,\"type\":\"hello\",\"client_name\":\"swbt-debug-client\","
                           "\"request_id\":\"h1\"}\n";
    return swbt_ipc_socket_send_all(socket, request, strlen(request)) == SWBT_IPC_SERVER_OK ? 0
                                                                                            : -1;
}

int swbt_debug_client_send_acquire(swbt_ipc_socket_t *socket) {
    const char request[] =
        "{\"v\":1,\"type\":\"acquire\",\"mode\":\"exclusive\",\"request_id\":\"a1\"}\n";
    return swbt_ipc_socket_send_all(socket, request, strlen(request)) == SWBT_IPC_SERVER_OK ? 0
                                                                                            : -1;
}

static int swbt_debug_client_send_format(swbt_ipc_socket_t *socket, const char *format, ...) {
    char request[SWBT_IPC_JSON_LINE_MAX];
    va_list args;
    int written = 0;

    va_start(args, format);
    // C11 Annex K formatting functions are not consistently available in the target toolchains.
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    written = vsnprintf(request, sizeof(request), format, args);
    va_end(args);

    if (written < 0 || (size_t)written >= sizeof(request)) {
        return -1;
    }
    return swbt_ipc_socket_send_all(socket, request, (size_t)written) == SWBT_IPC_SERVER_OK ? 0
                                                                                            : -1;
}

static int swbt_debug_client_io_send(swbt_debug_client_io_t *io, const char *data) {
    if (io == NULL || io->send == NULL || data == NULL) {
        return -1;
    }
    return io->send(io->context, data, strlen(data));
}

static int swbt_debug_client_io_send_format(swbt_debug_client_io_t *io, const char *format, ...) {
    char request[SWBT_IPC_JSON_LINE_MAX];
    va_list args;
    int written = 0;

    if (io == NULL || io->send == NULL || format == NULL) {
        return -1;
    }

    va_start(args, format);
    // C11 Annex K formatting functions are not consistently available in the target toolchains.
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    written = vsnprintf(request, sizeof(request), format, args);
    va_end(args);

    if (written < 0 || (size_t)written >= sizeof(request)) {
        return -1;
    }
    return io->send(io->context, request, (size_t)written);
}

static int swbt_debug_client_io_send_hello(swbt_debug_client_io_t *io) {
    return swbt_debug_client_io_send(
        io, "{\"v\":1,\"type\":\"hello\",\"client_name\":\"swbt-debug-client\","
            "\"request_id\":\"h1\"}\n");
}

static int swbt_debug_client_io_send_acquire(swbt_debug_client_io_t *io) {
    return swbt_debug_client_io_send(
        io, "{\"v\":1,\"type\":\"acquire\",\"mode\":\"exclusive\",\"request_id\":\"a1\"}\n");
}

static int swbt_debug_client_io_send_set_state(swbt_debug_client_io_t *io, const char *owner_id,
                                               const swbt_state_t *state) {
    if (owner_id == NULL || state == NULL) {
        return -1;
    }
    return swbt_debug_client_io_send_format(
        io,
        "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"%s\",\"seq\":%llu,"
        "\"request_id\":\"s1\",\"state\":{\"buttons\":%u,\"lx\":%u,\"ly\":%u,"
        "\"rx\":%u,\"ry\":%u,\"accel_x\":%d,\"accel_y\":%d,\"accel_z\":%d,"
        "\"gyro_x\":%d,\"gyro_y\":%d,\"gyro_z\":%d}}\n",
        owner_id, (unsigned long long)state->client_seq, (unsigned int)state->buttons,
        (unsigned int)state->lx, (unsigned int)state->ly, (unsigned int)state->rx,
        (unsigned int)state->ry, (int)state->accel_x, (int)state->accel_y, (int)state->accel_z,
        (int)state->gyro_x, (int)state->gyro_y, (int)state->gyro_z);
}

static int swbt_debug_client_io_send_get_status(swbt_debug_client_io_t *io) {
    return swbt_debug_client_io_send(io,
                                     "{\"v\":1,\"type\":\"get_status\",\"request_id\":\"g1\"}\n");
}

static int swbt_debug_client_io_send_release(swbt_debug_client_io_t *io, const char *owner_id) {
    if (owner_id == NULL) {
        return -1;
    }
    return swbt_debug_client_io_send_format(
        io, "{\"v\":1,\"type\":\"release\",\"owner_id\":\"%s\",\"request_id\":\"r1\"}\n", owner_id);
}

int swbt_debug_client_send_set_state(swbt_ipc_socket_t *socket, const char *owner_id,
                                     const swbt_state_t *state) {
    if (socket == NULL || owner_id == NULL || state == NULL) {
        return -1;
    }

    return swbt_debug_client_send_format(
        socket,
        "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"%s\",\"seq\":%llu,"
        "\"request_id\":\"s1\",\"state\":{\"buttons\":%u,\"lx\":%u,\"ly\":%u,"
        "\"rx\":%u,\"ry\":%u,\"accel_x\":%d,\"accel_y\":%d,\"accel_z\":%d,"
        "\"gyro_x\":%d,\"gyro_y\":%d,\"gyro_z\":%d}}\n",
        owner_id, (unsigned long long)state->client_seq, (unsigned int)state->buttons,
        (unsigned int)state->lx, (unsigned int)state->ly, (unsigned int)state->rx,
        (unsigned int)state->ry, (int)state->accel_x, (int)state->accel_y, (int)state->accel_z,
        (int)state->gyro_x, (int)state->gyro_y, (int)state->gyro_z);
}

int swbt_debug_client_send_get_status(swbt_ipc_socket_t *socket) {
    const char request[] = "{\"v\":1,\"type\":\"get_status\",\"request_id\":\"g1\"}\n";
    return swbt_ipc_socket_send_all(socket, request, strlen(request)) == SWBT_IPC_SERVER_OK ? 0
                                                                                            : -1;
}

int swbt_debug_client_send_release(swbt_ipc_socket_t *socket, const char *owner_id) {
    if (owner_id == NULL) {
        return -1;
    }
    return swbt_debug_client_send_format(
        socket, "{\"v\":1,\"type\":\"release\",\"owner_id\":\"%s\",\"request_id\":\"r1\"}\n",
        owner_id);
}

int swbt_debug_client_receive_response(swbt_ipc_socket_t *socket, char *buffer,
                                       size_t buffer_size) {
    size_t length = 0;

    if (socket == NULL || buffer == NULL || buffer_size == 0) {
        return -1;
    }

    while (true) {
        char byte = '\0';
        size_t received = 0;
        if (swbt_ipc_socket_receive(socket, &byte, 1, &received) != SWBT_IPC_SERVER_OK ||
            received != 1u) {
            buffer[0] = '\0';
            return -1;
        }
        if (length + 1u >= buffer_size) {
            buffer[0] = '\0';
            return -1;
        }
        buffer[length] = byte;
        ++length;
        if (byte == '\n') {
            buffer[length] = '\0';
            return 0;
        }
    }
}

int swbt_debug_client_response_string(const char *response, const char *key, char *out,
                                      size_t out_size) {
    char pattern[64];
    const char *start = NULL;
    const char *end = NULL;
    size_t key_length = 0;
    size_t length = 0;

    if (response == NULL || key == NULL || out == NULL || out_size == 0) {
        return -1;
    }

    key_length = strlen(key);
    if (key_length > sizeof(pattern) - 5u) {
        return -1;
    }
    pattern[0] = '"';
    for (size_t index = 0; index < key_length; ++index) {
        pattern[index + 1u] = key[index];
    }
    pattern[key_length + 1u] = '"';
    pattern[key_length + 2u] = ':';
    pattern[key_length + 3u] = '"';
    pattern[key_length + 4u] = '\0';

    start = strstr(response, pattern);
    if (start == NULL) {
        return -1;
    }
    start += strlen(pattern);
    end = strchr(start, '"');
    if (end == NULL) {
        return -1;
    }

    length = (size_t)(end - start);
    if (length + 1u > out_size) {
        return -1;
    }

    for (size_t index = 0; index < length; ++index) {
        out[index] = start[index];
    }
    out[length] = '\0';
    return 0;
}

static bool swbt_debug_client_response_contains(const char *response, const char *needle) {
    return response != NULL && strstr(response, needle) != NULL;
}

static int swbt_debug_client_receive_io(swbt_debug_client_io_t *io, char *response,
                                        size_t response_size) {
    if (io == NULL || io->receive == NULL) {
        return -1;
    }
    return io->receive(io->context, response, response_size);
}

static int swbt_debug_client_send_receive(swbt_debug_client_io_t *io,
                                          int (*send_request)(swbt_debug_client_io_t *),
                                          char *response, size_t response_size) {
    if (send_request(io) != 0) {
        return -1;
    }
    return swbt_debug_client_receive_io(io, response, response_size);
}

static int swbt_debug_client_release_best_effort(swbt_debug_client_io_t *io, const char *owner_id) {
    char response[SWBT_IPC_JSON_RESPONSE_MAX];
    if (owner_id == NULL || owner_id[0] == '\0') {
        return -1;
    }
    if (swbt_debug_client_io_send_release(io, owner_id) != 0) {
        return -1;
    }
    return swbt_debug_client_receive_io(io, response, sizeof(response));
}

int swbt_debug_client_run_io(const swbt_debug_client_config_t *config, swbt_debug_client_io_t *io,
                             FILE *out, FILE *err) {
    char response[SWBT_IPC_JSON_RESPONSE_MAX];
    char client_id[9] = "";
    char owner_id[9] = "";
    bool owner_acquired = false;

    if (config == NULL || io == NULL || out == NULL || err == NULL) {
        return 1;
    }

    if (swbt_debug_client_send_receive(io, swbt_debug_client_io_send_hello, response,
                                       sizeof(response)) != 0 ||
        !swbt_debug_client_response_contains(response, "\"type\":\"hello_ok\"") ||
        swbt_debug_client_response_string(response, "client_id", client_id, sizeof(client_id)) !=
            0) {
        swbt_debug_client_write_message(err, "hello failed\n");
        return 1;
    }

    if (swbt_debug_client_send_receive(io, swbt_debug_client_io_send_acquire, response,
                                       sizeof(response)) != 0) {
        swbt_debug_client_write_message(err, "acquire failed\n");
        return 1;
    }
    if (swbt_debug_client_response_contains(response, "\"type\":\"error\"")) {
        fputs(response, err);
        return 1;
    }
    if (!swbt_debug_client_response_contains(response, "\"type\":\"acquired\"") ||
        swbt_debug_client_response_string(response, "owner_id", owner_id, sizeof(owner_id)) != 0) {
        swbt_debug_client_write_message(err, "acquire response is invalid\n");
        return 1;
    }
    owner_acquired = true;

    if (swbt_debug_client_io_send_set_state(io, owner_id, &config->state) != 0 ||
        swbt_debug_client_receive_io(io, response, sizeof(response)) != 0) {
        if (owner_acquired) {
            (void)swbt_debug_client_release_best_effort(io, owner_id);
        }
        swbt_debug_client_write_message(err, "set_state failed\n");
        return 1;
    }
    if (swbt_debug_client_response_contains(response, "\"type\":\"error\"")) {
        (void)swbt_debug_client_release_best_effort(io, owner_id);
        fputs(response, err);
        return 1;
    }

    if (swbt_debug_client_send_receive(io, swbt_debug_client_io_send_get_status, response,
                                       sizeof(response)) != 0 ||
        swbt_debug_client_response_contains(response, "\"type\":\"error\"")) {
        (void)swbt_debug_client_release_best_effort(io, owner_id);
        swbt_debug_client_write_message(err, "get_status failed\n");
        return 1;
    }
    fputs(response, out);

    swbt_debug_client_sleep_ms(config->hold_ms);
    if (config->skip_release) {
        return 0;
    }

    if (swbt_debug_client_io_send_release(io, owner_id) != 0 ||
        swbt_debug_client_receive_io(io, response, sizeof(response)) != 0 ||
        swbt_debug_client_response_contains(response, "\"type\":\"error\"")) {
        swbt_debug_client_write_message(err, "release failed\n");
        return 1;
    }

    return 0;
}

static int swbt_debug_client_socket_io_send(void *context, const char *data, size_t size) {
    return swbt_ipc_socket_send_all((swbt_ipc_socket_t *)context, data, size) == SWBT_IPC_SERVER_OK
               ? 0
               : -1;
}

static int swbt_debug_client_socket_io_receive(void *context, char *buffer, size_t buffer_size) {
    return swbt_debug_client_receive_response((swbt_ipc_socket_t *)context, buffer, buffer_size);
}

int swbt_debug_client_main(int argc, const char *const *argv, FILE *out, FILE *err) {
    swbt_debug_client_config_t config;
    swbt_ipc_socket_t socket;
    swbt_debug_client_io_t io;
    int result = 0;

    if (swbt_debug_client_parse_args(argc, argv, &config, err) != 0) {
        return 2;
    }

    swbt_ipc_socket_init(&socket);
    if (swbt_ipc_socket_connect_loopback(&socket, config.port) != SWBT_IPC_SERVER_OK) {
        swbt_debug_client_write_message(err, "failed to connect to 127.0.0.1:");
        swbt_debug_client_write_u16(err, config.port);
        swbt_debug_client_write_message(err, "\n");
        return 3;
    }

    io.send = swbt_debug_client_socket_io_send;
    io.receive = swbt_debug_client_socket_io_receive;
    io.context = &socket;
    result = swbt_debug_client_run_io(&config, &io, out, err);
    swbt_ipc_socket_close(&socket);
    return result == 0 ? 0 : 4;
}
