// Boot code: vector table, image definition block, C runtime setup, clock init.
// Replaces the Pico SDK's crt0 and runtime init.
#include <stdint.h>
#include "board.h"

#define REG(addr) (*(volatile uint32_t *)(addr))

// Core registers
#define SCB_VTOR         REG(0xE000ED08u)

// Crystal oscillator and clock generators (RP2350 datasheet, clocks chapter)
#define XOSC_BASE        0x40048000u
#define XOSC_CTRL        REG(XOSC_BASE + 0x00)
#define XOSC_STATUS      REG(XOSC_BASE + 0x04)
#define XOSC_STARTUP     REG(XOSC_BASE + 0x0c)
#define XOSC_CTRL_FREQ_1_15MHZ 0xaa0u
#define XOSC_CTRL_ENABLE       (0xfabu << 12)
#define XOSC_STATUS_STABLE     (1u << 31)

#define CLOCKS_BASE      0x40010000u
#define CLK_REF_CTRL     REG(CLOCKS_BASE + 0x30)
#define CLK_REF_SELECTED REG(CLOCKS_BASE + 0x38)
#define CLK_SYS_CTRL     REG(CLOCKS_BASE + 0x3c)
#define CLK_SYS_SELECTED REG(CLOCKS_BASE + 0x44)
#define CLK_REF_SRC_XOSC 2u            // clk_ref source = crystal
#define CLK_SYS_SRC_REF  0u            // clk_sys source = clk_ref

extern uint32_t __data_load, __data_start, __data_end;
extern uint32_t __bss_start, __bss_end;
extern uint32_t __stack_top;
extern int main(void);

void reset_handler(void);
void default_handler(void) { for (;;) {} }

// Exceptions the kernel overrides; everything else falls back to default_handler.
#define WEAK_DEFAULT __attribute__((weak, alias("default_handler")))
void isr_nmi(void)         WEAK_DEFAULT;
void isr_hardfault(void)   WEAK_DEFAULT;
void isr_memmanage(void)   WEAK_DEFAULT;
void isr_busfault(void)    WEAK_DEFAULT;
void isr_usagefault(void)  WEAK_DEFAULT;
void isr_securefault(void) WEAK_DEFAULT;
void isr_svcall(void)      WEAK_DEFAULT;
void isr_debugmonitor(void) WEAK_DEFAULT;
void isr_pendsv(void)      WEAK_DEFAULT;
void isr_systick(void)     WEAK_DEFAULT;

typedef void (*vector_fn)(void);

// 16 core entries, then the 52 RP2350 external interrupts (all default for now).
__attribute__((section(".vectors"), used))
const vector_fn vector_table[16 + 52] = {
    (vector_fn)&__stack_top,   // initial main stack pointer
    reset_handler,
    isr_nmi,
    isr_hardfault,
    isr_memmanage,
    isr_busfault,
    isr_usagefault,
    isr_securefault,
    0, 0, 0,                   // reserved
    isr_svcall,
    isr_debugmonitor,
    0,                         // reserved
    isr_pendsv,
    isr_systick,
    [16 ... 16 + 51] = default_handler,
};

// Image definition block: tells the RP2350 bootrom this is a secure ARM
// executable for RP2350. Without it the bootrom will not run the image.
__attribute__((section(".embedded_block"), used))
const uint32_t image_def[] = {
    0xffffded3,   // block start marker
    0x10210142,   // item: IMAGE_TYPE = EXE, ARM, Secure, RP2350
    0x000001ff,   // item: last item, 1 word of items
    0x00000000,   // next block: offset 0 = loops back to this block
    0xab123579,   // block end marker
};

static void clock_init(void) {
    XOSC_STARTUP = 47;                                  // ~1 ms startup wait at 12 MHz (units of 256 cycles)
    XOSC_CTRL = XOSC_CTRL_ENABLE | XOSC_CTRL_FREQ_1_15MHZ;
    while (!(XOSC_STATUS & XOSC_STATUS_STABLE)) {}

    CLK_REF_CTRL = CLK_REF_SRC_XOSC;                    // clk_ref <- crystal
    while (!(CLK_REF_SELECTED & (1u << CLK_REF_SRC_XOSC))) {}

    CLK_SYS_CTRL = CLK_SYS_SRC_REF;                     // clk_sys <- clk_ref
    while (!(CLK_SYS_SELECTED & 1u)) {}
}

void reset_handler(void) {
    SCB_VTOR = (uint32_t)vector_table;

    // Copy initialized data from flash to RAM, zero the rest.
    uint32_t *src = &__data_load;
    for (uint32_t *dst = &__data_start; dst < &__data_end; ) *dst++ = *src++;
    for (uint32_t *p = &__bss_start; p < &__bss_end; ) *p++ = 0;

    clock_init();
    main();
    for (;;) {}
}
