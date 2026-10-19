#ifndef SYNC_H
#define SYNC_H
#include <stdint.h>
#include "kernel.h"

// ---- Counting semaphore ----
typedef struct { volatile uint32_t count; } sem_t;

void sem_init(sem_t *s, uint32_t initial);
int  sem_take(sem_t *s, uint32_t timeout_ms);   // 0 = got it, -1 = timed out
void sem_give(sem_t *s);                        // also safe to call from an interrupt handler

// ---- Message queue (fixed-size items copied in and out) ----
typedef struct {
    uint8_t *buf;
    uint32_t item_size, length, head, tail;
    sem_t items;    // how many items are in the queue
    sem_t space;    // how many free slots are left
} queue_t;

void queue_init(queue_t *q, void *buf, uint32_t item_size, uint32_t length);
int  queue_send(queue_t *q, const void *item, uint32_t timeout_ms);   // 0 ok, -1 timeout
int  queue_receive(queue_t *q, void *item, uint32_t timeout_ms);      // 0 ok, -1 timeout

// ---- Mutex with priority inheritance (not recursive) ----
typedef struct mutex { tcb_t *owner; } mutex_t;

void mutex_init(mutex_t *m);
int  mutex_lock(mutex_t *m);     // blocks; -1 if the caller already owns it
int  mutex_unlock(mutex_t *m);   // -1 if the caller is not the owner

#endif
