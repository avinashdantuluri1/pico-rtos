// Register-level GPIO driver for the RP2350 (output only for now).
#include "gpio.h"

#define REG(addr) (*(volatile uint32_t *)(addr))

#define RESETS_BASE      0x40020000u
#define IO_BANK0_BASE    0x40028000u
#define PADS_BANK0_BASE  0x40038000u
#define SIO_BASE         0xd0000000u

// Writing to base + 0x3000 clears the written bits (atomic alias)
#define ATOMIC_CLR       0x3000u

#define RESETS_RESET     REG(RESETS_BASE + 0x0)
#define RESETS_DONE      REG(RESETS_BASE + 0x8)
#define RESET_IO_BANK0   (1u << 6)
#define RESET_PADS_BANK0 (1u << 9)

#define IO_GPIO_CTRL(n)  REG(IO_BANK0_BASE + 0x04 + 8u * (n))
#define PADS_GPIO(n)     (PADS_BANK0_BASE + 0x04 + 4u * (n))
#define FUNCSEL_SIO      5u
#define PAD_ISO          (1u << 8)   // pad isolation, set out of reset on RP2350

#define SIO_GPIO_OE_SET  REG(SIO_BASE + 0x38)
#define SIO_GPIO_OUT_XOR REG(SIO_BASE + 0x28)

void gpio_init_output(uint32_t pin) {
    RESETS_RESET = RESETS_RESET & ~(RESET_IO_BANK0 | RESET_PADS_BANK0);
    while ((RESETS_DONE & (RESET_IO_BANK0 | RESET_PADS_BANK0)) !=
           (RESET_IO_BANK0 | RESET_PADS_BANK0)) {}

    REG(PADS_GPIO(pin) + ATOMIC_CLR) = PAD_ISO;  // un-isolate the pad
    IO_GPIO_CTRL(pin) = FUNCSEL_SIO;             // route pin to SIO
    SIO_GPIO_OE_SET = 1u << pin;                 // output enable
}

void gpio_toggle(uint32_t pin) {
    SIO_GPIO_OUT_XOR = 1u << pin;
}
