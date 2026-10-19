// Milestone 3: priority scheduler with sleep and an idle task. SysTick fires every
// 1 ms, wakes sleeping tasks, and pends PendSV when a switch may be needed;
// PendSV (context_switch.S) swaps tasks using kernel_pick_next().
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
#define SCB_SHCSR        REG(0xE000ED24u)
#define SCB_SHCSR_USGFAULTENA (1u << 18)
#define SCB_CFSR         REG(0xE000ED28u)
#define CFSR_STKOF       (1u << 20)
#define STACK_FILL       0xA5A5A5A5u
#define SCB_SHPR3        REG(0xE000ED20u)  // priorities: PendSV bits 23:16, SysTick 31:24

#include "board.h"
#define TICK_HZ          1000u
#define TIME_SLICE_TICKS 10u

#define MAX_TASKS        8          // includes the idle task
#define IDLE_STACK_WORDS 64
#define XPSR_THUMB       0x01000000u  // T bit must be 1 on Cortex-M

extern void kernel_start_first(uint32_t *sp) __attribute__((noreturn));

tcb_t *current_task;               // read by PendSV assembly (declared in kernel.h)
static tcb_t tcbs[MAX_TASKS];
static uint32_t idle_stack[IDLE_STACK_WORDS] __attribute__((aligned(8)));
static int num_tasks;
static volatile uint32_t ticks;

static void task_exit(void) {      // a task that returns lands here
    for (;;) {}
}

void kernel_init(void) {
    num_tasks = 0;
    current_task = 0;
}

uint32_t kernel_irq_save(void) {
    uint32_t primask;
    __asm volatile("mrs %0, primask\n cpsid i" : "=r"(primask) :: "memory");
    return primask;
}

void kernel_irq_restore(uint32_t primask) {
    __asm volatile("msr primask, %0" :: "r"(primask) : "memory");
}

static void idle_task(void) {
    for (;;) {
        __asm volatile("wfi");   // nothing to run: sleep until the next interrupt
    }
}

static int add_task(task_fn fn, uint32_t *stack_base, uint32_t stack_words, uint8_t priority) {
    if (num_tasks >= MAX_TASKS) return -1;

    // Fill with a known pattern so task_stack_unused_bytes() can find the high-water mark.
    for (uint32_t i = 0; i < stack_words; i++) stack_base[i] = STACK_FILL;

    // Full descending stack: top is the highest address, 8-byte aligned.
    uint32_t *sp = stack_base + stack_words;
    sp = (uint32_t *)((uint32_t)sp & ~7u);
    uint32_t *top = sp;

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
    t->stack_limit = (uint32_t *)(((uint32_t)stack_base + 7u) & ~7u);
    t->stack_top = top;
    t->priority = priority;
    t->base_priority = priority;
    t->has_timeout = 0;
    t->timed_out = 0;
    t->wait_obj = 0;
    t->wait_mutex = 0;
    t->state = TASK_READY;
    t->wake_tick = 0;
    return num_tasks++;
}

int task_create(task_fn fn, uint32_t *stack_base, uint32_t stack_words, uint8_t priority) {
    if (priority == 0) return -1;
    return add_task(fn, stack_base, stack_words, priority);
}

void kernel_start(void) {
    add_task(idle_task, idle_stack, IDLE_STACK_WORDS, 0);

    // Start the highest-priority task (first one created wins ties).
    tcb_t *first = &tcbs[0];
    for (int i = 1; i < num_tasks; i++) {
        if (tcbs[i].priority > first->priority) first = &tcbs[i];
    }
    current_task = first;

    SCB_SHCSR |= SCB_SHCSR_USGFAULTENA;   // report stack overflow as UsageFault, not HardFault

    // PendSV lowest priority so it never preempts another interrupt
    SCB_SHPR3 |= 0x00FF0000u;

    // Start the tick last: its handler needs current_task to be valid.
    SYST_RVR = SYS_CLK_HZ / TICK_HZ - 1;
    SYST_CVR = 0;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;

    kernel_start_first(current_task->sp);
}

uint32_t kernel_ticks(void) { return ticks; }

void kernel_yield(void) {
    SCB_ICSR = SCB_ICSR_PENDSVSET;             // give up the CPU now
    __asm volatile("dsb\n isb" ::: "memory");  // PendSV runs before we continue
}

void task_sleep(uint32_t ms) {
    if (ms > 0) {
        uint32_t primask = kernel_irq_save();
        current_task->wake_tick = ticks + ms;
        current_task->state = TASK_SLEEPING;
        kernel_irq_restore(primask);
    }
    kernel_yield();
}

// Mark the running task as waiting on obj. The caller must have interrupts off
// and call kernel_yield() after turning them back on.
void kernel_block_current(void *obj, uint32_t timeout_ms) {
    current_task->state = TASK_BLOCKED;
    current_task->wait_obj = obj;
    current_task->timed_out = 0;
    if (timeout_ms == WAIT_FOREVER) {
        current_task->has_timeout = 0;
    } else {
        current_task->has_timeout = 1;
        current_task->wake_tick = ticks + timeout_ms;
    }
}

// Wake the highest-priority task waiting on obj. Interrupts must be off.
// Requests a switch if the woken task outranks the running one.
int kernel_wake_one(void *obj) {
    tcb_t *best = 0;
    for (int i = 0; i < num_tasks; i++) {
        tcb_t *t = &tcbs[i];
        if (t->state == TASK_BLOCKED && t->wait_obj == obj &&
            (best == 0 || t->priority > best->priority)) {
            best = t;
        }
    }
    if (best == 0) return 0;
    best->state = TASK_READY;
    best->wait_obj = 0;
    best->wait_mutex = 0;
    best->has_timeout = 0;
    if (best->priority > current_task->priority) {
        SCB_ICSR = SCB_ICSR_PENDSVSET;
    }
    return 1;
}

tcb_t *kernel_tasks(int *count) {
    *count = num_tasks;
    return tcbs;
}

// Called from PendSV (assembly): run the highest-priority ready task.
// Scan starts after the current task, so equal priorities take turns.
void kernel_pick_next(void) {
    int cur = (int)(current_task - tcbs);
    tcb_t *best = 0;
    for (int i = 1; i <= num_tasks; i++) {
        tcb_t *t = &tcbs[(cur + i) % num_tasks];
        if (t->state == TASK_READY && (best == 0 || t->priority > best->priority)) {
            best = t;
        }
    }
    current_task = best;    // never null: the idle task is always ready
}

void isr_systick(void) {
    ticks++;
    int switch_needed = (ticks % TIME_SLICE_TICKS == 0);

    // Wake sleepers whose time is up (signed difference is safe across tick wraparound).
    for (int i = 0; i < num_tasks; i++) {
        tcb_t *t = &tcbs[i];
        int timer_wait = (t->state == TASK_SLEEPING) ||
                         (t->state == TASK_BLOCKED && t->has_timeout);
        if (timer_wait && (int32_t)(ticks - t->wake_tick) >= 0) {
            if (t->state == TASK_BLOCKED) {      // timeout, nobody woke it
                t->timed_out = 1;
                t->wait_obj = 0;
                t->has_timeout = 0;
            }
            t->state = TASK_READY;
            if (t->priority >= current_task->priority) switch_needed = 1;
        }
    }
    if (switch_needed) {
        SCB_ICSR = SCB_ICSR_PENDSVSET;
    }
}

uint32_t task_stack_unused_bytes(int task_index) {
    if (task_index < 0 || task_index >= num_tasks) return 0;
    tcb_t *t = &tcbs[task_index];
    uint32_t *p = t->stack_limit;
    while (p < t->stack_top && *p == STACK_FILL) p++;
    return (uint32_t)((uint8_t *)p - (uint8_t *)t->stack_limit);
}

// ---------- fault handling ----------

volatile fault_info_t kernel_fault_info;

__attribute__((weak)) void kernel_on_fault(void) {
    for (;;) {}
}

static void fault_stop(uint32_t cfsr) {
    uint32_t psp;
    __asm volatile("cpsid i\n mrs %0, psp" : "=r"(psp) :: "memory");
    kernel_fault_info.cfsr = cfsr;
    kernel_fault_info.psp = psp;
    kernel_fault_info.task_index = current_task ? (int32_t)(current_task - tcbs) : -1;
    kernel_on_fault();
    for (;;) {}
}

// Called from PendSV when saving a task's registers would run past its stack.
void kernel_stack_overflow(void) {
    fault_stop(CFSR_STKOF);
}

// The CPU raises this when PSP drops below PSPLIM (or exception stacking would).
void isr_usagefault(void) {
    fault_stop(SCB_CFSR);
}
