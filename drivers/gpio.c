// Register-level GPIO driver for the RP2350.
#include "gpio.h"
#include "rp2350.h"

#define IO_GPIO_CTRL(n)  REG(IO_BANK0_BASE + 0x04 + 8u * (n))
#define PADS_GPIO(n)     (PADS_BANK0_BASE + 0x04 + 4u * (n))
#define PAD_ISO          (1u << 8)   // pad isolation, set out of reset on RP2350

#define SIO_GPIO_OE_SET  REG(SIO_BASE + 0x38)
#define SIO_GPIO_OUT_XOR REG(SIO_BASE + 0x28)

// Connect a pin to one of its built-in functions (UART, SIO, ...).
void gpio_set_function(uint32_t pin, uint32_t func) {
    periph_unreset(RESET_IO_BANK0 | RESET_PADS_BANK0);
    REG(PADS_GPIO(pin) + ATOMIC_CLR) = PAD_ISO;  // un-isolate the pad
    IO_GPIO_CTRL(pin) = func;
}

void gpio_init_output(uint32_t pin) {
    gpio_set_function(pin, GPIO_FUNC_SIO);
    SIO_GPIO_OE_SET = 1u << pin;                 // output enable
}

void gpio_toggle(uint32_t pin) {
    SIO_GPIO_OUT_XOR = 1u << pin;
}
