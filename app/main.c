// Milestone 2: two tasks, each blinking its own LED, preempted by the kernel.
#include "gpio.h"
#include "kernel.h"

#define LED_A 15u
#define LED_B 14u
#define STACK_WORDS 256

static uint32_t stack_a[STACK_WORDS] __attribute__((aligned(8)));
static uint32_t stack_b[STACK_WORDS] __attribute__((aligned(8)));

static void delay_ms(uint32_t ms) {
    uint32_t start = kernel_ticks();
    while (kernel_ticks() - start < ms) {}   // busy wait; sleep() comes in Milestone 3
}

static void task_a(void) {
    for (;;) {
        gpio_toggle(LED_A);
        delay_ms(500);
    }
}

static void task_b(void) {
    for (;;) {
        gpio_toggle(LED_B);
        delay_ms(150);
    }
}

int main(void) {
    gpio_init_output(LED_A);
    gpio_init_output(LED_B);

    kernel_init();
    task_create(task_a, stack_a, STACK_WORDS);
    task_create(task_b, stack_b, STACK_WORDS);
    kernel_start();
}
