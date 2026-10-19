// Milestone 3: two tasks blinking their own LEDs using task_sleep(); the CPU idles in between.
#include "gpio.h"
#include "kernel.h"

#define LED_A 15u
#define LED_B 14u
#define STACK_WORDS 256

static uint32_t stack_a[STACK_WORDS] __attribute__((aligned(8)));
static uint32_t stack_b[STACK_WORDS] __attribute__((aligned(8)));

static void task_a(void) {
    for (;;) {
        gpio_toggle(LED_A);
        task_sleep(500);
    }
}

static void task_b(void) {
    for (;;) {
        gpio_toggle(LED_B);
        task_sleep(150);
    }
}

int main(void) {
    gpio_init_output(LED_A);
    gpio_init_output(LED_B);

    kernel_init();
    task_create(task_a, stack_a, STACK_WORDS, 1);
    task_create(task_b, stack_b, STACK_WORDS, 2);
    kernel_start();
}
