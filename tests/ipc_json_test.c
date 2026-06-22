#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ipc/ipc_json.h"
#include "ipc/ipc_session.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_rumble.h"

static int expect_contains(const char *text, const char *needle) {
    return strstr(text, needle) != NULL ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int handle(swbt_ipc_session_t *session, uint32_t client_id, const char *line, char *response,
                  size_t response_size) {
    for (size_t index = 0; index < response_size; ++index) {
        response[index] = '\0';
    }
    return swbt_ipc_json_handle_line(session, client_id, line, response, response_size);
}

int main(void) {
    swbt_ipc_session_t session;
    swbt_ipc_status_t status;
    char response[SWBT_IPC_JSON_RESPONSE_MAX];
    const uint8_t active_rumble[SWBT_SWITCH_RUMBLE_DATA_SIZE] = {
        0x04, 0x01, 0x80, 0x41, 0x08, 0x01, 0x80, 0x42,
    };

    if (swbt_ipc_session_init(&session) != SWBT_IPC_OK) {
        return 1;
    }

    if (handle(&session, 1001,
               "{\"v\":1,\"type\":\"hello\",\"client_name\":\"unit\",\"request_id\":\"h1\"}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 2;
    }
    if (expect_contains(response, "\"type\":\"hello_ok\"") ||
        expect_contains(response, "\"request_id\":\"h1\"") ||
        expect_contains(response, "\"client_id\":\"000003e9\"")) {
        return 3;
    }

    if (handle(&session, 1001,
               "{\"v\":1,\"type\":\"acquire\",\"mode\":\"exclusive\",\"request_id\":\"a1\"}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 4;
    }
    if (expect_contains(response, "\"type\":\"acquired\"") ||
        expect_contains(response, "\"request_id\":\"a1\"") ||
        expect_contains(response, "\"owner_id\":\"000003e9\"")) {
        return 5;
    }

    if (handle(&session, 2002,
               "{\"v\":1,\"type\":\"acquire\",\"mode\":\"exclusive\",\"request_id\":\"a2\"}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 6;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"owner_busy\"") ||
        expect_contains(response, "\"request_id\":\"a2\"")) {
        return 7;
    }

    if (handle(&session, 2002,
               "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"000007d2\",\"seq\":77,"
               "\"request_id\":\"s2\",\"state\":{\"buttons\":8,\"lx\":1234,\"ly\":2345,"
               "\"rx\":2048,\"ry\":2048,\"accel_x\":1,\"accel_y\":2,\"accel_z\":3,"
               "\"gyro_x\":4,\"gyro_y\":5,\"gyro_z\":6}}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 8;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"not_owner\"") ||
        expect_contains(response, "\"request_id\":\"s2\"")) {
        return 9;
    }

    if (handle(&session, 1001,
               "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"000003e9\",\"seq\":77,"
               "\"request_id\":\"s1\",\"state\":{\"buttons\":8,\"lx\":1234,\"ly\":2345,"
               "\"rx\":2048,\"ry\":2048,\"accel_x\":1,\"accel_y\":2,\"accel_z\":3,"
               "\"gyro_x\":4,\"gyro_y\":5,\"gyro_z\":6}}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 10;
    }
    if (expect_contains(response, "\"type\":\"state_accepted\"") ||
        expect_contains(response, "\"request_id\":\"s1\"") ||
        expect_contains(response, "\"seq\":77")) {
        return 11;
    }
    if (swbt_ipc_get_status(&session, &status) != SWBT_IPC_OK) {
        return 12;
    }
    if (expect_eq_u32(status.state.buttons, SWBT_BUTTON_A) ||
        expect_eq_u16(status.state.lx, 1234) || expect_eq_u16(status.state.ly, 2345) ||
        status.last_seq != 77) {
        return 13;
    }
    if (swbt_ipc_record_rumble(&session, active_rumble, 4242u) != SWBT_IPC_OK) {
        return 32;
    }

    if (handle(&session, 2002, "{\"v\":1,\"type\":\"get_status\",\"request_id\":\"g1\"}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 14;
    }
    if (expect_contains(response, "\"type\":\"status\"") ||
        expect_contains(response, "\"request_id\":\"g1\"") ||
        expect_contains(response, "\"present\":true") ||
        expect_contains(response, "\"owner_id\":\"000003e9\"") ||
        expect_contains(response, "\"last_seq\":77") ||
        expect_contains(response, "\"buttons\":8") || expect_contains(response, "\"lx\":1234") ||
        expect_contains(response, "\"rumble\":{\"updated\":true,\"last_update_ms\":4242,"
                                  "\"raw\":\"0401804108018042\"}")) {
        return 15;
    }

    if (handle(&session, 1001,
               "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"000003e9\",\"seq\":78,"
               "\"request_id\":\"bad-state\",\"state\":{\"buttons\":8,\"lx\":5000,\"ly\":2048,"
               "\"rx\":2048,\"ry\":2048,\"accel_x\":0,\"accel_y\":0,\"accel_z\":0,"
               "\"gyro_x\":0,\"gyro_y\":0,\"gyro_z\":0}}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 16;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"invalid_state\"") ||
        expect_contains(response, "\"request_id\":\"bad-state\"")) {
        return 17;
    }
    if (swbt_ipc_get_status(&session, &status) != SWBT_IPC_OK) {
        return 18;
    }
    if (expect_eq_u16(status.state.lx, 1234) || status.last_seq != 77) {
        return 19;
    }

    if (handle(&session, 1001,
               "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"000003e9\",\"seq\":79,"
               "\"request_id\":\"bad-state-object\",\"state\":0,\"buttons\":0,\"lx\":2048,"
               "\"ly\":2048,\"rx\":2048,\"ry\":2048,\"accel_x\":0,\"accel_y\":0,"
               "\"accel_z\":0,\"gyro_x\":0,\"gyro_y\":0,\"gyro_z\":0}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 20;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"invalid_state\"") ||
        expect_contains(response, "\"request_id\":\"bad-state-object\"")) {
        return 21;
    }
    if (swbt_ipc_get_status(&session, &status) != SWBT_IPC_OK) {
        return 22;
    }
    if (expect_eq_u16(status.state.lx, 1234) || status.last_seq != 77) {
        return 23;
    }

    if (handle(&session, 1001,
               "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"000003e9\",\"seq\":76,"
               "\"request_id\":\"stale\",\"state\":{\"buttons\":2,\"lx\":3456,\"ly\":2048,"
               "\"rx\":2048,\"ry\":2048,\"accel_x\":0,\"accel_y\":0,\"accel_z\":0,"
               "\"gyro_x\":0,\"gyro_y\":0,\"gyro_z\":0}}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 33;
    }
    if (expect_contains(response, "\"type\":\"state_accepted\"") ||
        expect_contains(response, "\"request_id\":\"stale\"") ||
        expect_contains(response, "\"seq\":76")) {
        return 34;
    }
    if (swbt_ipc_get_status(&session, &status) != SWBT_IPC_OK) {
        return 35;
    }
    if (expect_eq_u32(status.state.buttons, SWBT_BUTTON_A) ||
        expect_eq_u16(status.state.lx, 1234) || status.last_seq != 77) {
        return 36;
    }

    if (handle(&session, 1001,
               "{\"v\":1,\"type\":\"release\",\"owner_id\":\"000003e9\",\"request_id\":\"r1\"}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 24;
    }
    if (expect_contains(response, "\"type\":\"released\"") ||
        expect_contains(response, "\"request_id\":\"r1\"")) {
        return 25;
    }
    if (swbt_ipc_get_status(&session, &status) != SWBT_IPC_OK) {
        return 26;
    }
    if (status.has_owner || expect_eq_u32(status.state.buttons, 0) ||
        expect_eq_u16(status.state.lx, 2048)) {
        return 27;
    }

    if (handle(&session, 1001, "{\"v\":2,\"type\":\"hello\",\"request_id\":\"bad-v\"}\n", response,
               sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 28;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"invalid_version\"") ||
        expect_contains(response, "\"request_id\":\"bad-v\"")) {
        return 29;
    }

    if (handle(&session, 1001, "{\"v\":1,\"type\":\"hello\"", response, sizeof(response)) !=
        SWBT_IPC_JSON_OK) {
        return 30;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"invalid_json\"")) {
        return 31;
    }

    return 0;
}
