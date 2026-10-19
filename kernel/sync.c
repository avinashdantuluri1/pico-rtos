// Semaphores, queues and mutexes. A task that can't proceed marks itself BLOCKED on
// the object and yields; the object's "give"/"unlock" wakes the best waiter.
#include "sync.h"

// ---------- semaphore ----------

void sem_init(sem_t *s, uint32_t initial) {
    s->count = initial;
}

int sem_take(sem_t *s, uint32_t timeout_ms) {
    uint32_t deadline = kernel_ticks() + timeout_ms;
    for (;;) {
        uint32_t p = kernel_irq_save();
        if (s->count > 0) {
            s->count--;
            kernel_irq_restore(p);
            return 0;
        }
        uint32_t remaining = timeout_ms;
        if (timeout_ms != WAIT_FOREVER) {
            int32_t left = (int32_t)(deadline - kernel_ticks());
            if (left <= 0) {
                kernel_irq_restore(p);
                return -1;
            }
            remaining = (uint32_t)left;
        }
        kernel_block_current(s, remaining);
        kernel_irq_restore(p);
        kernel_yield();
        if (current_task->timed_out) return -1;
        // Woken by sem_give: loop and re-check, another task may have taken it first.
    }
}

void sem_give(sem_t *s) {
    uint32_t p = kernel_irq_save();
    s->count++;
    kernel_wake_one(s);
    kernel_irq_restore(p);
}

// ---------- queue (built from two semaphores) ----------

void queue_init(queue_t *q, void *buf, uint32_t item_size, uint32_t length) {
    q->buf = buf;
    q->item_size = item_size;
    q->length = length;
    q->head = q->tail = 0;
    sem_init(&q->items, 0);
    sem_init(&q->space, length);
}

int queue_send(queue_t *q, const void *item, uint32_t timeout_ms) {
    if (sem_take(&q->space, timeout_ms) != 0) return -1;   // full
    uint32_t p = kernel_irq_save();
    const uint8_t *src = item;
    uint8_t *dst = q->buf + q->head * q->item_size;
    for (uint32_t i = 0; i < q->item_size; i++) dst[i] = src[i];
    q->head = (q->head + 1) % q->length;
    kernel_irq_restore(p);
    sem_give(&q->items);
    return 0;
}

int queue_receive(queue_t *q, void *item, uint32_t timeout_ms) {
    if (sem_take(&q->items, timeout_ms) != 0) return -1;   // empty
    uint32_t p = kernel_irq_save();
    uint8_t *dst = item;
    const uint8_t *src = q->buf + q->tail * q->item_size;
    for (uint32_t i = 0; i < q->item_size; i++) dst[i] = src[i];
    q->tail = (q->tail + 1) % q->length;
    kernel_irq_restore(p);
    sem_give(&q->space);
    return 0;
}

// ---------- mutex with priority inheritance ----------

void mutex_init(mutex_t *m) {
    m->owner = 0;
}

int mutex_lock(mutex_t *m) {
    for (;;) {
        uint32_t p = kernel_irq_save();
        if (m->owner == current_task) {          // would deadlock on itself
            kernel_irq_restore(p);
            return -1;
        }
        if (m->owner == 0) {
            m->owner = current_task;
            kernel_irq_restore(p);
            return 0;
        }

        // Owned by someone else. Lift the owner (and whoever it is waiting on,
        // transitively) to at least our priority so a medium-priority task
        // can't keep it from running.
        uint8_t prio = current_task->priority;
        tcb_t *o = m->owner;
        for (int hop = 0; o != 0 && hop < 8; hop++) {
            if (o->priority < prio) o->priority = prio;
            if (o->state == TASK_BLOCKED && o->wait_mutex != 0) {
                o = o->wait_mutex->owner;
            } else {
                break;
            }
        }

        kernel_block_current(m, WAIT_FOREVER);
        current_task->wait_mutex = m;
        kernel_irq_restore(p);
        kernel_yield();
        // Woken by unlock: loop and try to take it.
    }
}

int mutex_unlock(mutex_t *m) {
    uint32_t p = kernel_irq_save();
    if (m->owner != current_task) {
        kernel_irq_restore(p);
        return -1;
    }
    m->owner = 0;

    // Drop back to our own priority, unless we still hold another mutex that a
    // higher-priority task is waiting for.
    int count;
    tcb_t *tasks = kernel_tasks(&count);
    uint8_t prio = current_task->base_priority;
    for (int i = 0; i < count; i++) {
        tcb_t *t = &tasks[i];
        if (t->state == TASK_BLOCKED && t->wait_mutex != 0 &&
            t->wait_mutex->owner == current_task && t->priority > prio) {
            prio = t->priority;
        }
    }
    current_task->priority = prio;

    kernel_wake_one(m);     // requests a switch if the waiter outranks us now
    kernel_irq_restore(p);
    return 0;
}
