#ifndef GPIO_H
#define GPIO_H
#include <stdint.h>

#define GPIO_FUNC_UART 2u    // UART0 on GP0/GP1
#define GPIO_FUNC_SIO  5u    // plain software-controlled pin

void gpio_set_function(uint32_t pin, uint32_t func);
void gpio_init_output(uint32_t pin);
void gpio_toggle(uint32_t pin);

#endif
