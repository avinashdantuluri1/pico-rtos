// Demo: a producer sends numbers through a queue to a consumer. Each side toggles
// its own LED, and both touch a shared counter under a mutex. A third task runs a
// serial shell on UART0 (GP0 = TX, GP1 = RX, 115200 baud).
#include "gpio.h"
#include "kernel.h"
#include "sync.h"
#include "uart.h"
#include "shell.h"

#define LED_PRODUCER 14u
#define LED_CONSUMER 15u
#define STACK_WORDS 256

// Set to 1 to add a task that overflows its stack on purpose after ~3 s.
// Expected: the kernel stops and both LEDs flash rapidly (see kernel_on_fault below).
#define STACK_OVERFLOW_TEST 0

static uint32_t stack_producer[STACK_WORDS] __attribute__((aligned(8)));
static uint32_t stack_consumer[STACK_WORDS] __attribute__((aligned(8)));
static uint32_t stack_shell[512] __attribute__((aligned(8)));

static queue_t queue;
static uint32_t queue_storage[4];
static mutex_t counter_lock;
static uint32_t shared_counter;

static void producer(void) {
    uint32_t n = 0;
    for (;;) {
        queue_send(&queue, &n, WAIT_FOREVER);
        n++;
        gpio_toggle(LED_PRODUCER);

        mutex_lock(&counter_lock);
        shared_counter++;
        mutex_unlock(&counter_lock);

        task_sleep(300);
    }
}

static void consumer(void) {
    uint32_t value;
    for (;;) {
        if (queue_receive(&queue, &value, WAIT_FOREVER) == 0) {
            gpio_toggle(LED_CONSUMER);

            mutex_lock(&counter_lock);
            shared_counter++;
            mutex_unlock(&counter_lock);
        }
    }
}

// Runs with interrupts off after the kernel catches a fault: flash both LEDs fast.
void kernel_on_fault(void) {
    for (;;) {
        gpio_toggle(LED_PRODUCER);
        gpio_toggle(LED_CONSUMER);
        for (volatile uint32_t i = 0; i < 200000; i++) {}
    }
}

#if STACK_OVERFLOW_TEST
static uint32_t stack_bad[128] __attribute__((aligned(8)));

static uint32_t eat_stack(uint32_t depth) {
    volatile uint32_t pad[32];          // 128 bytes per call
    pad[0] = depth;
    return depth ? eat_stack(depth - 1) + pad[0] : 0;
}

static void bad_task(void) {
    task_sleep(3000);
    eat_stack(100);                     // needs ~13 KB, the stack is 512 bytes
    for (;;) {}
}
#endif

int main(void) {
    gpio_init_output(LED_PRODUCER);
    gpio_init_output(LED_CONSUMER);
    uart_init(115200);

    kernel_init();
    queue_init(&queue, queue_storage, sizeof(uint32_t), 4);
    mutex_init(&counter_lock);

    task_create(producer, stack_producer, STACK_WORDS, 2, "producer");
    task_create(consumer, stack_consumer, STACK_WORDS, 1, "consumer");
    task_create(shell_task, stack_shell, 512, 1, "shell");
#if STACK_OVERFLOW_TEST
    task_create(bad_task, stack_bad, 128, 1, "bad");
#endif
    kernel_start();
}
