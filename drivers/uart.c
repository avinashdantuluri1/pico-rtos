// Register-level driver for UART0 (an ARM PL011). Polling only, no interrupts yet.
#include "uart.h"
#include "gpio.h"
#include "rp2350.h"
#include "board.h"

#define UART0_BASE   0x40070000u
#define UARTDR       REG(UART0_BASE + 0x00)   // data: write to send, read to receive
#define UARTFR       REG(UART0_BASE + 0x18)   // flags
#define UARTIBRD     REG(UART0_BASE + 0x24)   // baud divisor, whole part
#define UARTFBRD     REG(UART0_BASE + 0x28)   // baud divisor, fraction part
#define UARTLCR_H    REG(UART0_BASE + 0x2c)   // line control
#define UARTCR       REG(UART0_BASE + 0x30)   // control

#define FR_TXFF      (1u << 5)    // transmit FIFO full
#define FR_RXFE      (1u << 4)    // receive FIFO empty
#define LCR_H_WLEN8  (3u << 5)    // 8 data bits
#define LCR_H_FEN    (1u << 4)    // use the FIFOs
#define CR_UARTEN    (1u << 0)
#define CR_TXE       (1u << 8)
#define CR_RXE       (1u << 9)

// Peripheral clock: the UART needs it switched on, sourced from clk_sys.
#define CLK_PERI_CTRL   REG(CLOCKS_BASE + 0x48)
#define CLK_PERI_ENABLE (1u << 11)
#define CLK_PERI_AUX_SYS (0u << 5)

void uart_init(uint32_t baud) {
    CLK_PERI_CTRL = CLK_PERI_ENABLE | CLK_PERI_AUX_SYS;
    periph_unreset(RESET_UART0);

    gpio_set_function(0, GPIO_FUNC_UART);   // GP0 = TX
    gpio_set_function(1, GPIO_FUNC_UART);   // GP1 = RX

    // divisor = clock / (16 * baud), stored as a whole part and 1/64ths
    uint32_t div = (8u * SYS_CLK_HZ) / baud;   // = 128 * (clock / (16 * baud)), so the low 7 bits are the fraction
    UARTIBRD = div >> 7;
    UARTFBRD = ((div & 0x7f) + 1) / 2;
    UARTLCR_H = LCR_H_WLEN8 | LCR_H_FEN;       // writing this latches the divisor

    UARTCR = CR_UARTEN | CR_TXE | CR_RXE;
}

void uart_putc(char c) {
    while (UARTFR & FR_TXFF) {}
    UARTDR = (uint32_t)(uint8_t)c;
}

void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

void uart_put_dec(uint32_t value) {
    char buf[10];
    int n = 0;
    do {
        buf[n++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    while (n) uart_putc(buf[--n]);
}

void uart_put_hex(uint32_t value) {
    uart_puts("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        uint32_t nibble = (value >> shift) & 0xf;
        uart_putc((char)(nibble < 10 ? '0' + nibble : 'a' + nibble - 10));
    }
}

int uart_try_getc(void) {
    if (UARTFR & FR_RXFE) return -1;
    return (int)(UARTDR & 0xff);
}
