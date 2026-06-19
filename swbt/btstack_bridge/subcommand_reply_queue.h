#ifndef SWBT_BTSTACK_BRIDGE_SUBCOMMAND_REPLY_QUEUE_H
#define SWBT_BTSTACK_BRIDGE_SUBCOMMAND_REPLY_QUEUE_H

#include <stddef.h>
#include <stdint.h>

#include "switch/switch_subcommand_reply.h"

#define SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_CAPACITY 4u

typedef enum {
    SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK = 0,
    SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_EMPTY = 1,
    SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_ARGUMENT = -1,
    SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_FULL = -2,
    SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_REPORT_SIZE = -3,
    SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_SEND_FAILED = -4,
} swbt_btstack_subcommand_reply_queue_result_t;

typedef int (*swbt_btstack_subcommand_reply_send_callback_t)(void *context, uint16_t hid_cid,
                                                             const uint8_t *report,
                                                             size_t report_size);

typedef struct {
    uint16_t hid_cid;
    uint8_t report[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE];
    size_t report_size;
} swbt_btstack_subcommand_reply_queue_item_t;

typedef struct {
    swbt_btstack_subcommand_reply_queue_item_t items[SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_CAPACITY];
    size_t head;
    size_t count;
} swbt_btstack_subcommand_reply_queue_t;

swbt_btstack_subcommand_reply_queue_result_t
swbt_btstack_subcommand_reply_queue_init(swbt_btstack_subcommand_reply_queue_t *queue);

size_t swbt_btstack_subcommand_reply_queue_size(const swbt_btstack_subcommand_reply_queue_t *queue);

swbt_btstack_subcommand_reply_queue_result_t
swbt_btstack_subcommand_reply_queue_enqueue(swbt_btstack_subcommand_reply_queue_t *queue,
                                            uint16_t hid_cid, const uint8_t *report,
                                            size_t report_size);

swbt_btstack_subcommand_reply_queue_result_t swbt_btstack_subcommand_reply_queue_send_next(
    swbt_btstack_subcommand_reply_queue_t *queue,
    swbt_btstack_subcommand_reply_send_callback_t send_callback, void *send_context);

#endif
