#ifndef SWBT_CORE_SPIN_LOCK_H
#define SWBT_CORE_SPIN_LOCK_H

#include <stdbool.h>
#include <stdatomic.h>

typedef struct {
    atomic_bool locked;
} swbt_spin_lock_t;

static inline void swbt_spin_lock_init(swbt_spin_lock_t *lock) {
    atomic_init(&lock->locked, false);
}

static inline void swbt_spin_lock_acquire(swbt_spin_lock_t *lock) {
    bool expected = false;
    while (!atomic_compare_exchange_weak_explicit(&lock->locked, &expected, true,
                                                  memory_order_acquire, memory_order_relaxed)) {
        expected = false;
    }
}

static inline void swbt_spin_lock_release(swbt_spin_lock_t *lock) {
    atomic_store_explicit(&lock->locked, false, memory_order_release);
}

#endif
