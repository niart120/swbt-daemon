#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/subcommand_reply_queue.h"
#include "switch/switch_subcommand_reply.h"

typedef struct {
    int calls;
    int result;
    uint16_t hid_cid;
    uint8_t report[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE];
    size_t report_size;
} send_capture_t;

typedef struct {
    uint8_t seed;
    size_t report_size;
} report_pattern_t;

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_size(size_t actual, size_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_bytes(const uint8_t *actual, const uint8_t *expected, size_t len) {
    for (size_t index = 0; index < len; ++index) {
        if (actual[index] != expected[index]) {
            return 1;
        }
    }
    return 0;
}

static int capture_send(void *context, uint16_t hid_cid, const uint8_t *report,
                        size_t report_size) {
    send_capture_t *capture = (send_capture_t *)context;
    capture->calls += 1;
    capture->hid_cid = hid_cid;
    capture->report_size = report_size;
    for (size_t index = 0; index < report_size && index < sizeof(capture->report); ++index) {
        capture->report[index] = report[index];
    }
    return capture->result;
}

static void fill_report(uint8_t *report, report_pattern_t pattern) {
    for (size_t index = 0; index < pattern.report_size; ++index) {
        report[index] = (uint8_t)(pattern.seed + index);
    }
}

static int enqueue_then_send_preserves_cid_size_and_bytes(void) {
    swbt_btstack_subcommand_reply_queue_t queue;
    send_capture_t capture = {0};
    uint8_t report[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE] = {0};
    fill_report(report, (report_pattern_t){.seed = 0x21u, .report_size = sizeof(report)});

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_subcommand_reply_queue_init(&queue),
                            SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    failed += expect_eq_int(
        swbt_btstack_subcommand_reply_queue_enqueue(&queue, 0x0042u, report, sizeof(report)),
        SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    report[0] = 0xFFu;
    failed += expect_eq_size(swbt_btstack_subcommand_reply_queue_size(&queue), 1u);
    failed +=
        expect_eq_int(swbt_btstack_subcommand_reply_queue_send_next(&queue, capture_send, &capture),
                      SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    failed += expect_eq_int(capture.calls, 1);
    failed += expect_eq_u16(capture.hid_cid, 0x0042u);
    failed += expect_eq_size(capture.report_size, SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    report[0] = 0x21u;
    failed += expect_bytes(capture.report, report, sizeof(report));
    failed += expect_eq_size(swbt_btstack_subcommand_reply_queue_size(&queue), 0u);
    return failed;
}

static int multiple_replies_are_sent_fifo(void) {
    swbt_btstack_subcommand_reply_queue_t queue;
    send_capture_t capture = {0};
    uint8_t first[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE] = {0};
    uint8_t second[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE] = {0};
    fill_report(first, (report_pattern_t){.seed = 0x10u, .report_size = sizeof(first)});
    fill_report(second, (report_pattern_t){.seed = 0x40u, .report_size = sizeof(second)});

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_subcommand_reply_queue_init(&queue),
                            SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    failed += expect_eq_int(
        swbt_btstack_subcommand_reply_queue_enqueue(&queue, 0x1001u, first, sizeof(first)),
        SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    failed += expect_eq_int(
        swbt_btstack_subcommand_reply_queue_enqueue(&queue, 0x1002u, second, sizeof(second)),
        SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);

    failed +=
        expect_eq_int(swbt_btstack_subcommand_reply_queue_send_next(&queue, capture_send, &capture),
                      SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    failed += expect_eq_u16(capture.hid_cid, 0x1001u);
    failed += expect_bytes(capture.report, first, sizeof(first));

    failed +=
        expect_eq_int(swbt_btstack_subcommand_reply_queue_send_next(&queue, capture_send, &capture),
                      SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    failed += expect_eq_u16(capture.hid_cid, 0x1002u);
    failed += expect_bytes(capture.report, second, sizeof(second));
    failed += expect_eq_size(swbt_btstack_subcommand_reply_queue_size(&queue), 0u);
    return failed;
}

static int full_queue_does_not_overwrite_existing_items(void) {
    swbt_btstack_subcommand_reply_queue_t queue;
    send_capture_t capture = {0};
    uint8_t report[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE] = {0};
    uint8_t overflow[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE] = {0};
    fill_report(report, (report_pattern_t){.seed = 0x01u, .report_size = sizeof(report)});
    fill_report(overflow, (report_pattern_t){.seed = 0x80u, .report_size = sizeof(overflow)});

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_subcommand_reply_queue_init(&queue),
                            SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    for (size_t index = 0; index < SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_CAPACITY; ++index) {
        report[1] = (uint8_t)index;
        failed += expect_eq_int(swbt_btstack_subcommand_reply_queue_enqueue(
                                    &queue, (uint16_t)(0x2000u + index), report, sizeof(report)),
                                SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    }
    failed += expect_eq_int(
        swbt_btstack_subcommand_reply_queue_enqueue(&queue, 0x2999u, overflow, sizeof(overflow)),
        SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_FULL);
    failed += expect_eq_size(swbt_btstack_subcommand_reply_queue_size(&queue),
                             SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_CAPACITY);

    for (size_t index = 0; index < SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_CAPACITY; ++index) {
        failed += expect_eq_int(
            swbt_btstack_subcommand_reply_queue_send_next(&queue, capture_send, &capture),
            SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
        failed += expect_eq_u16(capture.hid_cid, (uint16_t)(0x2000u + index));
        failed += expect_eq_u8(capture.report[1], (uint8_t)index);
        failed += expect_eq_int(capture.report[0] == overflow[0], 0);
    }
    return failed;
}

static int send_failure_leaves_head_for_retry(void) {
    swbt_btstack_subcommand_reply_queue_t queue;
    send_capture_t capture = {.result = -7};
    uint8_t report[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE] = {0};
    fill_report(report, (report_pattern_t){.seed = 0x33u, .report_size = sizeof(report)});

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_subcommand_reply_queue_init(&queue),
                            SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    failed += expect_eq_int(
        swbt_btstack_subcommand_reply_queue_enqueue(&queue, 0x0043u, report, sizeof(report)),
        SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    failed +=
        expect_eq_int(swbt_btstack_subcommand_reply_queue_send_next(&queue, capture_send, &capture),
                      SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_SEND_FAILED);
    failed += expect_eq_size(swbt_btstack_subcommand_reply_queue_size(&queue), 1u);

    capture.result = 0;
    failed +=
        expect_eq_int(swbt_btstack_subcommand_reply_queue_send_next(&queue, capture_send, &capture),
                      SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    failed += expect_eq_u16(capture.hid_cid, 0x0043u);
    failed += expect_bytes(capture.report, report, sizeof(report));
    failed += expect_eq_size(swbt_btstack_subcommand_reply_queue_size(&queue), 0u);
    return failed;
}

static int invalid_arguments_and_report_size_are_rejected(void) {
    swbt_btstack_subcommand_reply_queue_t queue;
    uint8_t report[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE + 1u] = {0};

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_subcommand_reply_queue_init(NULL),
                            SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_subcommand_reply_queue_init(&queue),
                            SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK);
    failed += expect_eq_int(swbt_btstack_subcommand_reply_queue_enqueue(
                                NULL, 0x0042u, report, SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE),
                            SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_subcommand_reply_queue_enqueue(
                                &queue, 0x0042u, NULL, SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE),
                            SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_ARGUMENT);
    failed +=
        expect_eq_int(swbt_btstack_subcommand_reply_queue_enqueue(&queue, 0x0042u, report, 0u),
                      SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_REPORT_SIZE);
    failed += expect_eq_int(
        swbt_btstack_subcommand_reply_queue_enqueue(&queue, 0x0042u, report, sizeof(report)),
        SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_REPORT_SIZE);
    failed += expect_eq_int(swbt_btstack_subcommand_reply_queue_send_next(NULL, capture_send, NULL),
                            SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_subcommand_reply_queue_send_next(&queue, NULL, NULL),
                            SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_ARGUMENT);
    failed +=
        expect_eq_int(swbt_btstack_subcommand_reply_queue_send_next(&queue, capture_send, NULL),
                      SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_EMPTY);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += enqueue_then_send_preserves_cid_size_and_bytes();
    failed += multiple_replies_are_sent_fifo();
    failed += full_queue_does_not_overwrite_existing_items();
    failed += send_failure_leaves_head_for_retry();
    failed += invalid_arguments_and_report_size_are_rejected();
    return failed == 0 ? 0 : 1;
}
