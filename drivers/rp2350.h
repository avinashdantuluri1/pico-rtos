#ifndef RP2350_H
#define RP2350_H
// Register addresses and bits shared by the drivers (RP2350 datasheet).
#include <stdint.h>

#define REG(addr) (*(volatile uint32_t *)(addr))

#define RESETS_BASE      0x40020000u
#define IO_BANK0_BASE    0x40028000u
#define PADS_BANK0_BASE  0x40038000u
#define CLOCKS_BASE      0x40010000u
#define SIO_BASE         0xd0000000u

// Writing to base + 0x3000 clears the written bits (atomic alias)
#define ATOMIC_CLR       0x3000u

#define RESETS_RESET     REG(RESETS_BASE + 0x0)
#define RESETS_DONE      REG(RESETS_BASE + 0x8)
#define RESET_IO_BANK0   (1u << 6)
#define RESET_PADS_BANK0 (1u << 9)
#define RESET_UART0      (1u << 26)

// Take the blocks in `mask` out of reset and wait until they are ready.
static inline void periph_unreset(uint32_t mask) {
    RESETS_RESET = RESETS_RESET & ~mask;
    while ((RESETS_DONE & mask) != mask) {}
}

#endif
