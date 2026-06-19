#include "btstack_bridge/subcommand_reply_queue.h"

static void clear_item(swbt_btstack_subcommand_reply_queue_item_t *item) {
    item->hid_cid = 0u;
    item->report_size = 0u;
    for (size_t index = 0; index < sizeof(item->report); ++index) {
        item->report[index] = 0u;
    }
}

static size_t enqueue_index(const swbt_btstack_subcommand_reply_queue_t *queue) {
    return (queue->head + queue->count) % SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_CAPACITY;
}

swbt_btstack_subcommand_reply_queue_result_t
swbt_btstack_subcommand_reply_queue_init(swbt_btstack_subcommand_reply_queue_t *queue) {
    if (queue == NULL) {
        return SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_ARGUMENT;
    }

    queue->head = 0u;
    queue->count = 0u;
    for (size_t index = 0; index < SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_CAPACITY; ++index) {
        clear_item(&queue->items[index]);
    }
    return SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK;
}

size_t
swbt_btstack_subcommand_reply_queue_size(const swbt_btstack_subcommand_reply_queue_t *queue) {
    return queue == NULL ? 0u : queue->count;
}

swbt_btstack_subcommand_reply_queue_result_t
swbt_btstack_subcommand_reply_queue_enqueue(swbt_btstack_subcommand_reply_queue_t *queue,
                                            uint16_t hid_cid, const uint8_t *report,
                                            size_t report_size) {
    if (queue == NULL || report == NULL) {
        return SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_ARGUMENT;
    }
    if (report_size == 0u || report_size > SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE) {
        return SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_REPORT_SIZE;
    }
    if (queue->count >= SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_CAPACITY) {
        return SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_FULL;
    }

    swbt_btstack_subcommand_reply_queue_item_t *item = &queue->items[enqueue_index(queue)];
    item->hid_cid = hid_cid;
    item->report_size = report_size;
    for (size_t index = 0; index < report_size; ++index) {
        item->report[index] = report[index];
    }
    for (size_t index = report_size; index < sizeof(item->report); ++index) {
        item->report[index] = 0u;
    }
    queue->count += 1u;
    return SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK;
}

swbt_btstack_subcommand_reply_queue_result_t swbt_btstack_subcommand_reply_queue_send_next(
    swbt_btstack_subcommand_reply_queue_t *queue,
    swbt_btstack_subcommand_reply_send_callback_t send_callback, void *send_context) {
    if (queue == NULL || send_callback == NULL) {
        return SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_INVALID_ARGUMENT;
    }
    if (queue->count == 0u) {
        return SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_EMPTY;
    }

    swbt_btstack_subcommand_reply_queue_item_t *item = &queue->items[queue->head];
    const int send_result =
        send_callback(send_context, item->hid_cid, item->report, item->report_size);
    if (send_result != 0) {
        return SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_ERROR_SEND_FAILED;
    }

    clear_item(item);
    queue->head = (queue->head + 1u) % SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_CAPACITY;
    queue->count -= 1u;
    return SWBT_BTSTACK_SUBCOMMAND_REPLY_QUEUE_OK;
}
