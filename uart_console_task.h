/*
 * @file uart_console_task.h
 * Exposes UART prebios function, and any other required functions from the
 * UART Console task
 */

/*
 * PreOS Task for UART console. Sets up uart instance for data transmission,
 * and creates UART console task.
 * UART device on linux will be ttyACM0.
 * This code MUST be called before the BIOS is started.
 */
void uart_prebios(void);

