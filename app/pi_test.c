// Priority inheritance test. Three tasks share one mutex:
//   low    (priority 1) takes the mutex at once and does a long job while holding it
//   high   (priority 3) wakes at 100 ms and needs the mutex
//   medium (priority 2) wakes at 200 ms and hogs the CPU for a long time
// With priority inheritance, low is boosted above medium while high waits, so low
// finishes and releases the mutex long before medium finishes. Without it, medium
// keeps interrupting low and finishes first.
//
// Result is shown on the LEDs once high gets the mutex:
//   PASS (medium not finished yet): the two LEDs alternate slowly
//   FAIL (medium finished first):   both LEDs blink together, fast
//   INVALID (mutex was free when high asked, so nothing was tested): only LED A blinks slowly
#include "gpio.h"
#include "kernel.h"
#include "sync.h"

#define LED_A 15u
#define LED_B 14u
#define STACK_WORDS 256

#define LOW_WORK    4000000u    // must take well over 100 ms, so low still holds the mutex
#define MEDIUM_WORK 4000000u

static uint32_t stack_low[STACK_WORDS]    __attribute__((aligned(8)));
static uint32_t stack_medium[STACK_WORDS] __attribute__((aligned(8)));
static uint32_t stack_high[STACK_WORDS]   __attribute__((aligned(8)));

static mutex_t lock;
static volatile uint32_t medium_done;

enum { RESULT_PASS = 1, RESULT_FAIL = 2, RESULT_INVALID = 3 };

static void spin(uint32_t n) {
    for (volatile uint32_t i = 0; i < n; i++) {}
}

static void low_task(void) {
    mutex_lock(&lock);
    spin(LOW_WORK);
    mutex_unlock(&lock);
    for (;;) task_sleep(1000);
}

static void medium_task(void) {
    task_sleep(200);
    spin(MEDIUM_WORK);
    medium_done = 1;
    for (;;) task_sleep(1000);
}

static void high_task(void) {
    task_sleep(100);
    int contended = (lock.owner != 0);      // did low still hold it when we asked?
    mutex_lock(&lock);                      // blocks until low releases it
    uint32_t medium_had_finished = medium_done;
    mutex_unlock(&lock);

    uint32_t result = !contended ? RESULT_INVALID
                    : medium_had_finished ? RESULT_FAIL
                    : RESULT_PASS;

    gpio_toggle(LED_A);                     // start with A on, B off
    for (;;) {
        if (result == RESULT_PASS) {
            gpio_toggle(LED_A);
            gpio_toggle(LED_B);
            task_sleep(500);
        } else if (result == RESULT_FAIL) {
            gpio_toggle(LED_A);
            gpio_toggle(LED_B);
            task_sleep(100);
        } else {
            gpio_toggle(LED_A);
            task_sleep(500);
        }
    }
}

int main(void) {
    gpio_init_output(LED_A);
    gpio_init_output(LED_B);

    kernel_init();
    mutex_init(&lock);
    task_create(low_task,    stack_low,    STACK_WORDS, 1, "low");
    task_create(medium_task, stack_medium, STACK_WORDS, 2, "medium");
    task_create(high_task,   stack_high,   STACK_WORDS, 3, "high");
    kernel_start();
}
