// Milestone 4 demo: a producer sends numbers through a queue to a consumer.
// Each side toggles its own LED, and both touch a shared counter under a mutex.
#include "gpio.h"
#include "kernel.h"
#include "sync.h"

#define LED_PRODUCER 14u
#define LED_CONSUMER 15u
#define STACK_WORDS 256

static uint32_t stack_producer[STACK_WORDS] __attribute__((aligned(8)));
static uint32_t stack_consumer[STACK_WORDS] __attribute__((aligned(8)));

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

int main(void) {
    gpio_init_output(LED_PRODUCER);
    gpio_init_output(LED_CONSUMER);

    kernel_init();
    queue_init(&queue, queue_storage, sizeof(uint32_t), 4);
    mutex_init(&counter_lock);

    task_create(producer, stack_producer, STACK_WORDS, 2);
    task_create(consumer, stack_consumer, STACK_WORDS, 1);
    kernel_start();
}
