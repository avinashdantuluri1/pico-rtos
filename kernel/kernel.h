#ifndef KERNEL_H
#define KERNEL_H
#include <stdint.h>

typedef void (*task_fn)(void);

typedef enum { TASK_READY, TASK_SLEEPING } task_state_t;

// Task control block. The saved stack pointer MUST stay the first member:
// the assembly in context_switch.S loads/stores it at offset 0.
typedef struct tcb {
    uint32_t *sp;
    uint32_t wake_tick;     // when sleeping: tick at which to become ready
    uint8_t priority;       // higher number = higher priority; 0 is the idle task
    uint8_t state;
} tcb_t;

void kernel_init(void);
// priority must be 1 or higher (0 is reserved for the idle task)
int  task_create(task_fn fn, uint32_t *stack_base, uint32_t stack_words, uint8_t priority);
void kernel_start(void) __attribute__((noreturn));
void task_sleep(uint32_t ms);
uint32_t kernel_ticks(void);

#endif
