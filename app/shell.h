#ifndef SHELL_H
#define SHELL_H

// Task body for the serial shell. Call uart_init() before the kernel starts,
// then create a task that runs shell_task.
void shell_task(void);

#endif
