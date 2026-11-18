// A tiny command shell over UART0. You type a command, press Enter, and it answers.
// Commands: help, uptime, tasks
#include "shell.h"
#include "kernel.h"
#include "uart.h"

#define LINE_MAX 64

static int str_equal(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

// Print a string, then spaces until it is `width` characters wide (for table columns).
static void put_padded(const char *s, int width) {
    int n = 0;
    while (*s) { uart_putc(*s++); n++; }
    while (n++ < width) uart_putc(' ');
}

static void put_dec_padded(uint32_t value, int width) {
    int digits = 1;
    for (uint32_t v = value; v >= 10; v /= 10) digits++;
    uart_put_dec(value);
    while (digits++ < width) uart_putc(' ');
}

static const char *state_name(const tcb_t *t) {
    if (t == current_task) return "running";
    switch (t->state) {
        case TASK_READY:    return "ready";
        case TASK_SLEEPING: return "sleeping";
        case TASK_BLOCKED:  return "blocked";
        default:            return "?";
    }
}

static void cmd_tasks(void) {
    int count;
    tcb_t *tasks = kernel_tasks(&count);
    uart_puts("ID  NAME      PRIO  STATE     STACK FREE (bytes)\r\n");
    for (int i = 0; i < count; i++) {
        put_dec_padded((uint32_t)i, 4);
        put_padded(tasks[i].name ? tasks[i].name : "?", 10);
        put_dec_padded(tasks[i].priority, 6);
        put_padded(state_name(&tasks[i]), 10);
        uart_put_dec(task_stack_unused_bytes(i));
        uart_puts("\r\n");
    }
}

static void run_command(const char *line) {
    if (line[0] == '\0') return;
    if (str_equal(line, "help")) {
        uart_puts("help    show this list\r\n");
        uart_puts("uptime  milliseconds since boot\r\n");
        uart_puts("tasks   list every task: priority, state, free stack\r\n");
    } else if (str_equal(line, "uptime")) {
        uart_put_dec(kernel_ticks());
        uart_puts(" ms\r\n");
    } else if (str_equal(line, "tasks")) {
        cmd_tasks();
    } else {
        uart_puts("unknown command, type help\r\n");
    }
}

void shell_task(void) {
    char line[LINE_MAX];
    int len = 0;

    uart_puts("\r\npico-rtos shell. Type help.\r\n> ");
    for (;;) {
        int c = uart_try_getc();
        if (c < 0) {
            task_sleep(2);          // nothing typed: let other tasks run
            continue;
        }
        if (c == '\r' || c == '\n') {
            uart_puts("\r\n");
            line[len] = '\0';
            run_command(line);
            len = 0;
            uart_puts("> ");
        } else if (c == 0x08 || c == 0x7f) {      // backspace
            if (len > 0) {
                len--;
                uart_puts("\b \b");
            }
        } else if (c >= 0x20 && c < 0x7f && len < LINE_MAX - 1) {
            line[len++] = (char)c;
            uart_putc((char)c);                   // echo what was typed
        }
    }
}
