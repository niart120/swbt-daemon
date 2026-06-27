#include "btstack_bridge/device.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int steps[8];
    size_t step_count;
    int platform_start_calls;
    int platform_stop_calls;
    int hid_register_calls;
    int hid_stop_calls;
    int connect_calls;
    int send_calls;
    int hid_register_result;
    int connect_result;
    int send_result;
    uint16_t hid_cid;
    uint8_t send_message[8];
    size_t send_message_size;
    uint8_t connect_address[6];
    uint16_t connect_control_psm;
    uint16_t connect_interrupt_psm;
    uint8_t *service_buffer;
    size_t service_buffer_size;
    swbt_btstack_hid_registration_config_t registration_config;
} fake_device_port_t;

enum {
    STEP_PLATFORM_START = 1,
    STEP_HID_REGISTER = 2,
    STEP_HID_STOP = 3,
    STEP_PLATFORM_STOP = 4,
};

static void record_step(fake_device_port_t *fake, int step) {
    if (fake->step_count < sizeof(fake->steps) / sizeof(fake->steps[0])) {
        fake->steps[fake->step_count] = step;
    }
    fake->step_count += 1u;
}

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_size(size_t actual, size_t expected) {
    return actual == expected ? 0 : 1;
}

static int fake_platform_start(void *context) {
    fake_device_port_t *fake = context;
    fake->platform_start_calls += 1;
    record_step(fake, STEP_PLATFORM_START);
    return 0;
}

static void fake_platform_stop(void *context) {
    fake_device_port_t *fake = context;
    fake->platform_stop_calls += 1;
    record_step(fake, STEP_PLATFORM_STOP);
}

static int fake_hid_register(void *context, uint8_t *service_buffer, size_t service_buffer_size,
                             const swbt_btstack_hid_registration_config_t *config) {
    fake_device_port_t *fake = context;
    fake->hid_register_calls += 1;
    fake->service_buffer = service_buffer;
    fake->service_buffer_size = service_buffer_size;
    if (config != NULL) {
        fake->registration_config = *config;
    }
    record_step(fake, STEP_HID_REGISTER);
    return fake->hid_register_result;
}

static void fake_hid_stop(void *context) {
    fake_device_port_t *fake = context;
    fake->hid_stop_calls += 1;
    record_step(fake, STEP_HID_STOP);
}

static int fake_connect(void *context, const swbt_btstack_device_connect_request_t *request,
                        uint16_t *out_hid_cid) {
    fake_device_port_t *fake = context;
    fake->connect_calls += 1;
    if (request != NULL) {
        for (size_t index = 0u; index < sizeof(fake->connect_address); ++index) {
            fake->connect_address[index] = request->address[index];
        }
        fake->connect_control_psm = request->control_psm;
        fake->connect_interrupt_psm = request->interrupt_psm;
    }
    if (out_hid_cid != NULL) {
        *out_hid_cid = 0x0042u;
    }
    return fake->connect_result;
}

static int fake_send(void *context, uint16_t hid_cid, const uint8_t *message, size_t message_size) {
    fake_device_port_t *fake = context;
    fake->send_calls += 1;
    fake->hid_cid = hid_cid;
    fake->send_message_size = message_size;
    for (size_t index = 0u; index < message_size && index < sizeof(fake->send_message); ++index) {
        fake->send_message[index] = message[index];
    }
    return fake->send_result;
}

static swbt_btstack_device_port_t fake_port(void) {
    return (swbt_btstack_device_port_t){
        .platform_start = fake_platform_start,
        .platform_stop = fake_platform_stop,
        .hid_register = fake_hid_register,
        .hid_stop = fake_hid_stop,
        .connect = fake_connect,
        .send = fake_send,
    };
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): BTstack packet handler ABI.
static void fake_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet,
                                uint16_t size) {
    (void)packet_type;
    (void)channel;
    (void)packet;
    (void)size;
}

static swbt_btstack_hid_registration_config_t sample_registration_config(void) {
    return (swbt_btstack_hid_registration_config_t){
        .device_name = "Pro Controller",
        .packet_handler = fake_packet_handler,
    };
}

static int open_starts_platform_then_registers_hid_and_close_reverses_order(void) {
    fake_device_port_t fake = {0};
    uint8_t service_buffer[32] = {0};
    swbt_btstack_device_t device;
    const swbt_btstack_device_port_t port = fake_port();
    const swbt_btstack_hid_registration_config_t registration = sample_registration_config();
    const int expected_steps[] = {
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
    };

    int failed = 0;
    failed +=
        expect_eq_int(swbt_btstack_device_init(&device, &port, &fake), SWBT_BTSTACK_DEVICE_OK);
    failed +=
        expect_eq_int(swbt_btstack_device_open(&device,
                                               (swbt_btstack_device_open_options_t){
                                                   .service_buffer = service_buffer,
                                                   .service_buffer_size = sizeof(service_buffer),
                                                   .registration = &registration,
                                               }),
                      SWBT_BTSTACK_DEVICE_OK);
    swbt_btstack_device_close(&device);

    failed += expect_eq_int(fake.platform_start_calls, 1);
    failed += expect_eq_int(fake.hid_register_calls, 1);
    failed += expect_true(fake.service_buffer == service_buffer);
    failed += expect_eq_size(fake.service_buffer_size, sizeof(service_buffer));
    failed += expect_true(fake.registration_config.packet_handler == registration.packet_handler);
    failed += expect_eq_int(fake.hid_stop_calls, 1);
    failed += expect_eq_int(fake.platform_stop_calls, 1);
    failed += expect_eq_size(fake.step_count, sizeof(expected_steps) / sizeof(expected_steps[0]));
    for (size_t index = 0u; index < sizeof(expected_steps) / sizeof(expected_steps[0]); ++index) {
        failed += expect_eq_int(fake.steps[index], expected_steps[index]);
    }
    return failed;
}

static int open_failure_closes_started_platform_without_hid_stop(void) {
    fake_device_port_t fake = {
        .hid_register_result = -7,
    };
    uint8_t service_buffer[32] = {0};
    swbt_btstack_device_t device;
    const swbt_btstack_device_port_t port = fake_port();
    const swbt_btstack_hid_registration_config_t registration = sample_registration_config();

    int failed = 0;
    failed +=
        expect_eq_int(swbt_btstack_device_init(&device, &port, &fake), SWBT_BTSTACK_DEVICE_OK);
    failed +=
        expect_eq_int(swbt_btstack_device_open(&device,
                                               (swbt_btstack_device_open_options_t){
                                                   .service_buffer = service_buffer,
                                                   .service_buffer_size = sizeof(service_buffer),
                                                   .registration = &registration,
                                               }),
                      SWBT_BTSTACK_DEVICE_ERROR_OPEN_FAILED);
    failed += expect_eq_int(fake.platform_start_calls, 1);
    failed += expect_eq_int(fake.hid_register_calls, 1);
    failed += expect_eq_int(fake.hid_stop_calls, 0);
    failed += expect_eq_int(fake.platform_stop_calls, 1);
    return failed;
}

static int connect_forwards_request_to_port(void) {
    fake_device_port_t fake = {0};
    uint8_t service_buffer[32] = {0};
    swbt_btstack_device_t device;
    const swbt_btstack_device_port_t port = fake_port();
    const swbt_btstack_hid_registration_config_t registration = sample_registration_config();
    const swbt_btstack_device_connect_request_t request = {
        .address = {0xABu, 0x89u, 0x67u, 0x45u, 0x23u, 0x01u},
        .control_psm = SWBT_BTSTACK_DEVICE_HID_CONTROL_PSM,
        .interrupt_psm = SWBT_BTSTACK_DEVICE_HID_INTERRUPT_PSM,
    };
    uint16_t hid_cid = 0u;

    int failed = 0;
    failed +=
        expect_eq_int(swbt_btstack_device_init(&device, &port, &fake), SWBT_BTSTACK_DEVICE_OK);
    failed +=
        expect_eq_int(swbt_btstack_device_open(&device,
                                               (swbt_btstack_device_open_options_t){
                                                   .service_buffer = service_buffer,
                                                   .service_buffer_size = sizeof(service_buffer),
                                                   .registration = &registration,
                                               }),
                      SWBT_BTSTACK_DEVICE_OK);
    failed += expect_eq_int(swbt_btstack_device_connect(&device, &request, &hid_cid),
                            SWBT_BTSTACK_DEVICE_OK);
    failed += expect_eq_int(fake.connect_calls, 1);
    failed += expect_eq_u16(hid_cid, 0x0042u);
    failed += expect_eq_u16(fake.connect_control_psm, SWBT_BTSTACK_DEVICE_HID_CONTROL_PSM);
    failed += expect_eq_u16(fake.connect_interrupt_psm, SWBT_BTSTACK_DEVICE_HID_INTERRUPT_PSM);
    for (size_t index = 0u; index < sizeof(request.address); ++index) {
        failed += expect_eq_u8(fake.connect_address[index], request.address[index]);
    }
    swbt_btstack_device_close(&device);
    return failed;
}

static int send_forwards_raw_interrupt_message_bytes(void) {
    const uint8_t message[] = {0xA1u, 0x30u, 0x41u, 0x8Eu, 0x08u};
    fake_device_port_t fake = {0};
    uint8_t service_buffer[32] = {0};
    swbt_btstack_device_t device;
    const swbt_btstack_device_port_t port = fake_port();
    const swbt_btstack_hid_registration_config_t registration = sample_registration_config();

    int failed = 0;
    failed +=
        expect_eq_int(swbt_btstack_device_init(&device, &port, &fake), SWBT_BTSTACK_DEVICE_OK);
    failed +=
        expect_eq_int(swbt_btstack_device_open(&device,
                                               (swbt_btstack_device_open_options_t){
                                                   .service_buffer = service_buffer,
                                                   .service_buffer_size = sizeof(service_buffer),
                                                   .registration = &registration,
                                               }),
                      SWBT_BTSTACK_DEVICE_OK);
    failed += expect_eq_int(swbt_btstack_device_send(&device, 0x0042u, message, sizeof(message)),
                            SWBT_BTSTACK_DEVICE_OK);
    failed += expect_eq_int(fake.send_calls, 1);
    failed += expect_eq_u16(fake.hid_cid, 0x0042u);
    failed += expect_eq_size(fake.send_message_size, sizeof(message));
    for (size_t index = 0u; index < sizeof(message); ++index) {
        failed += expect_eq_u8(fake.send_message[index], message[index]);
    }
    swbt_btstack_device_close(&device);
    return failed;
}

static int recv_decodes_hid_connection_opened_packet(void) {
    fake_device_port_t fake = {0};
    uint8_t service_buffer[32] = {0};
    uint8_t opened_event[] = {0xEFu, 13u,   0x02u, 0x42u, 0x00u, 0x00u, 0xABu, 0x89u,
                              0x67u, 0x45u, 0x23u, 0x01u, 0u,    0u,    1u};
    swbt_btstack_device_t device;
    swbt_btstack_hid_event_t event;
    const swbt_btstack_device_port_t port = fake_port();
    const swbt_btstack_hid_registration_config_t registration = sample_registration_config();

    int failed = 0;
    failed +=
        expect_eq_int(swbt_btstack_device_init(&device, &port, &fake), SWBT_BTSTACK_DEVICE_OK);
    failed +=
        expect_eq_int(swbt_btstack_device_open(&device,
                                               (swbt_btstack_device_open_options_t){
                                                   .service_buffer = service_buffer,
                                                   .service_buffer_size = sizeof(service_buffer),
                                                   .registration = &registration,
                                               }),
                      SWBT_BTSTACK_DEVICE_OK);
    failed += expect_eq_int(
        swbt_btstack_device_recv(&device, 0x04u, opened_event, sizeof(opened_event), &event),
        SWBT_BTSTACK_DEVICE_OK);
    failed += expect_eq_int((int)event.type, (int)SWBT_BTSTACK_HID_EVENT_CONNECTION_OPENED);
    failed += expect_eq_u16(event.hid_cid, 0x0042u);
    failed += expect_eq_u8(event.status, 0x00u);
    failed += expect_eq_u8(event.address[0], 0x01u);
    failed += expect_eq_u8(event.address[5], 0xABu);
    swbt_btstack_device_close(&device);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += open_starts_platform_then_registers_hid_and_close_reverses_order();
    failed += open_failure_closes_started_platform_without_hid_stop();
    failed += connect_forwards_request_to_port();
    failed += send_forwards_raw_interrupt_message_bytes();
    failed += recv_decodes_hid_connection_opened_packet();
    return failed == 0 ? 0 : 1;
}
