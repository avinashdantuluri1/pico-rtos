#ifndef UART_H
#define UART_H
#include <stdint.h>

void uart_init(uint32_t baud);            // UART0: TX on GP0, RX on GP1, 8N1
void uart_putc(char c);                   // waits if the transmit FIFO is full
void uart_puts(const char *s);
void uart_put_dec(uint32_t value);
void uart_put_hex(uint32_t value);
int  uart_try_getc(void);                 // next received byte, or -1 if none

#endif
