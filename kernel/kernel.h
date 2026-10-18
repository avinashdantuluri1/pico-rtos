#ifndef KERNEL_H
#define KERNEL_H
#include <stdint.h>

typedef void (*task_fn)(void);

// Task control block. The saved stack pointer MUST stay the first member:
// the assembly in context_switch.S loads/stores it at offset 0.
typedef struct tcb {
    uint32_t *sp;
    struct tcb *next;
} tcb_t;

void kernel_init(void);
int  task_create(task_fn fn, uint32_t *stack_base, uint32_t stack_words);
void kernel_start(void) __attribute__((noreturn));
uint32_t kernel_ticks(void);

#endif
