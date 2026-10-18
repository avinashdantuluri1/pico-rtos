// Milestone 0: blink an external LED from our own SysTick handler.
// No HAL: every hardware access below is a raw register read/write.
#include <stdint.h>

#define REG(addr) (*(volatile uint32_t *)(addr))

// RP2350 peripherals (datasheet section 2.2 address map)
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

#define LED_PIN          15u
#define IO_GPIO_CTRL(n)  REG(IO_BANK0_BASE + 0x04 + 8u * (n))
#define PADS_GPIO(n)     (PADS_BANK0_BASE + 0x04 + 4u * (n))
#define FUNCSEL_SIO      5u
#define PAD_ISO          (1u << 8)   // pad isolation, set out of reset on RP2350

#define SIO_GPIO_OE_SET  REG(SIO_BASE + 0x38)
#define SIO_GPIO_OUT_XOR REG(SIO_BASE + 0x28)

// ARM Cortex-M33 SysTick (in the core, same on every Cortex-M)
#define SYST_CSR         REG(0xE000E010u)
#define SYST_RVR         REG(0xE000E014u)
#define SYST_CVR         REG(0xE000E018u)
#define SYST_CSR_ENABLE    (1u << 0)
#define SYST_CSR_TICKINT   (1u << 1)
#define SYST_CSR_CLKSOURCE (1u << 2)  // 1 = processor clock

#define SYS_CLK_HZ       150000000u  // SDK boot code sets clk_sys to 150 MHz
#define TICK_HZ          1000u

static volatile uint32_t tick_count;

// Name comes from the Pico SDK's vector table (crt0.S); our definition
// overrides the SDK's weak default.
void isr_systick(void) {
    tick_count++;
    if (tick_count % 500 == 0) {
        SIO_GPIO_OUT_XOR = 1u << LED_PIN;   // toggle LED every 500 ms
    }
}

static void gpio_init_output(uint32_t pin) {
    RESETS_RESET = RESETS_RESET & ~(RESET_IO_BANK0 | RESET_PADS_BANK0);
    while ((RESETS_DONE & (RESET_IO_BANK0 | RESET_PADS_BANK0)) !=
           (RESET_IO_BANK0 | RESET_PADS_BANK0)) {}

    REG(PADS_GPIO(pin) + ATOMIC_CLR) = PAD_ISO;  // un-isolate the pad
    IO_GPIO_CTRL(pin) = FUNCSEL_SIO;             // route pin to SIO
    SIO_GPIO_OE_SET = 1u << pin;                 // output enable
}

static void systick_init(void) {
    SYST_RVR = SYS_CLK_HZ / TICK_HZ - 1;  // counts N..0, so N+1 clocks per tick
    SYST_CVR = 0;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
}

int main(void) {
    gpio_init_output(LED_PIN);
    systick_init();
    for (;;) {
        __asm volatile("wfi");  // sleep until next interrupt
    }
}
