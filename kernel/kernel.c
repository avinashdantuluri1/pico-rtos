// Milestone 2: round-robin kernel. SysTick fires every 1 ms and pends PendSV
// every TIME_SLICE_TICKS; PendSV (context_switch.S) swaps tasks.
#include "kernel.h"

#define REG(addr) (*(volatile uint32_t *)(addr))

#define SYST_CSR         REG(0xE000E010u)
#define SYST_RVR         REG(0xE000E014u)
#define SYST_CVR         REG(0xE000E018u)
#define SYST_CSR_ENABLE    (1u << 0)
#define SYST_CSR_TICKINT   (1u << 1)
#define SYST_CSR_CLKSOURCE (1u << 2)

#define SCB_ICSR         REG(0xE000ED04u)
#define SCB_ICSR_PENDSVSET (1u << 28)
#define SCB_SHPR3        REG(0xE000ED20u)  // priorities: PendSV bits 23:16, SysTick 31:24

#include "board.h"
#define TICK_HZ          1000u
#define TIME_SLICE_TICKS 10u

#define MAX_TASKS        4
#define XPSR_THUMB       0x01000000u  // T bit must be 1 on Cortex-M

extern void kernel_start_first(uint32_t *sp) __attribute__((noreturn));

tcb_t *current_task;               // read by PendSV assembly
static tcb_t tcbs[MAX_TASKS];
static int num_tasks;
static volatile uint32_t ticks;

static void task_exit(void) {      // a task that returns lands here
    for (;;) {}
}

void kernel_init(void) {
    num_tasks = 0;
    current_task = 0;
}

int task_create(task_fn fn, uint32_t *stack_base, uint32_t stack_words) {
    if (num_tasks >= MAX_TASKS) return -1;

    // Full descending stack: top is the highest address, 8-byte aligned.
    uint32_t *sp = stack_base + stack_words;
    sp = (uint32_t *)((uint32_t)sp & ~7u);

    // Frame the CPU pops on exception return (low -> high address: r0..xPSR)
    *--sp = XPSR_THUMB;            // xPSR
    *--sp = (uint32_t)fn;          // PC  (bit 0 is already set for Thumb functions)
    *--sp = (uint32_t)task_exit;   // LR
    *--sp = 0;                     // r12
    *--sp = 0;                     // r3
    *--sp = 0;                     // r2
    *--sp = 0;                     // r1
    *--sp = 0;                     // r0
    // Registers PendSV saves/restores by hand
    for (int i = 0; i < 8; i++) *--sp = 0;   // r11..r4

    tcb_t *t = &tcbs[num_tasks];
    t->sp = sp;
    // circular list: new task points at the first, previous last points at new
    t->next = &tcbs[0];
    if (num_tasks > 0) tcbs[num_tasks - 1].next = t;
    return num_tasks++;
}

void kernel_start(void) {
    // PendSV lowest priority so it never preempts another interrupt
    SCB_SHPR3 |= 0x00FF0000u;

    SYST_RVR = SYS_CLK_HZ / TICK_HZ - 1;
    SYST_CVR = 0;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;

    current_task = &tcbs[0];
    kernel_start_first(current_task->sp);
}

uint32_t kernel_ticks(void) { return ticks; }

// Called from PendSV (assembly): choose the next task, round robin.
void kernel_pick_next(void) {
    current_task = current_task->next;
}

void isr_systick(void) {
    ticks++;
    if (ticks % TIME_SLICE_TICKS == 0) {
        SCB_ICSR = SCB_ICSR_PENDSVSET;   // request a context switch
    }
}
