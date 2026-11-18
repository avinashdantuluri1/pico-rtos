// First hardware test: no kernel, no interrupts. Proves that the image boots,
// the GPIO driver works and the LEDs are wired right. GP14 and GP15 blink
// alternately using a plain busy-wait loop (timing is approximate).
#include "gpio.h"

#define LED_A 15u
#define LED_B 14u

static void busy_wait(void) {
    for (volatile uint32_t i = 0; i < 600000; i++) {}
}

int main(void) {
    gpio_init_output(LED_A);
    gpio_init_output(LED_B);
    gpio_toggle(LED_A);          // start with A on, B off
    for (;;) {
        gpio_toggle(LED_A);
        gpio_toggle(LED_B);
        busy_wait();
    }
}
