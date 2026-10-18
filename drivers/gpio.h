#ifndef GPIO_H
#define GPIO_H
#include <stdint.h>

void gpio_init_output(uint32_t pin);
void gpio_toggle(uint32_t pin);

#endif
