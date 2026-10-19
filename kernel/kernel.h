#ifndef KERNEL_H
#define KERNEL_H
#include <stdint.h>

typedef void (*task_fn)(void);

struct mutex;

typedef enum { TASK_READY, TASK_SLEEPING, TASK_BLOCKED } task_state_t;

#define WAIT_FOREVER 0xFFFFFFFFu

// Task control block. The saved stack pointer MUST stay first and stack_limit
// second: the assembly in context_switch.S uses offsets 0 and 4.
typedef struct tcb {
    uint32_t *sp;
    uint32_t *stack_limit;  // lowest usable stack address (loaded into PSPLIM); offset 4 is used by assembly
    uint32_t *stack_top;    // one past the highest stack address
    uint32_t wake_tick;     // sleeping/blocked-with-timeout: tick at which to become ready
    uint8_t priority;       // effective priority (may be boosted by priority inheritance)
    uint8_t base_priority;  // priority the task was created with; 0 is the idle task
    uint8_t state;
    uint8_t has_timeout;    // blocked with a timeout
    uint8_t timed_out;      // set when a blocked task was woken by its timeout
    void *wait_obj;         // what a blocked task is waiting on
    struct mutex *wait_mutex; // set only when waiting on a mutex (for inheritance)
} tcb_t;

void kernel_init(void);
// priority must be 1 or higher (0 is reserved for the idle task)
int  task_create(task_fn fn, uint32_t *stack_base, uint32_t stack_words, uint8_t priority);
void kernel_start(void) __attribute__((noreturn));
void task_sleep(uint32_t ms);

// Bytes of a task's stack that have never been used (high-water mark), by creation index.
uint32_t task_stack_unused_bytes(int task_index);

// Filled in when a stack overflow or other usage fault stops the system.
typedef struct {
    uint32_t cfsr;          // Configurable Fault Status Register (bit 20 = stack overflow)
    uint32_t psp;           // task stack pointer at the time
    int32_t  task_index;    // which task was running
} fault_info_t;
extern volatile fault_info_t kernel_fault_info;
// Called (with interrupts off) after a fault is recorded. Weak: the application can
// override it to signal the fault. The default just halts.
void kernel_on_fault(void);
uint32_t kernel_ticks(void);

// Building blocks for sync.c (semaphores, queues, mutexes)
extern tcb_t *current_task;
uint32_t kernel_irq_save(void);
void kernel_irq_restore(uint32_t primask);
void kernel_yield(void);
void kernel_block_current(void *obj, uint32_t timeout_ms);   // call with interrupts off
int  kernel_wake_one(void *obj);                              // call with interrupts off
tcb_t *kernel_tasks(int *count);

#endif
