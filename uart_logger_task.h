/**
 * @file uart_logger.h
 * Implements the task responsible for reading data from a UART, and writing it
 * to an SD card log file.
 *
 * Pins Required:
 * PD6- UART RX
 * PD7- UART TX
 */

#include <stdbool.h>
#include <ti/sysbios/knl/Queue.h>

/*
 * PreOS Task for UART logger. Sets up uart instance for data transmission,
 * This code MUST be called before the BIOS is started.
 */
void uart_logger_prebios(void);

/**
 * Enables UART log forwarding. After this function is called, get_log_char()
 * will work as expected, since the logger task will be enqueuing data.
 */
void enable_log_forwarding(void);

/**
 * Disables UART log forwarding. After this function is called, get_log_char()
 * will stop working, since data will not longer be enqueued.
 */
void disable_log_forwarding(void);

/**
 * Gets a char of data from the UART logger's queue. Useful if another task
 * wants to monitor the UART logger, outside of the SD card writes.
 * Notes:
 * enable_log_forwarding() should be called to alert the logger task to enqueue
 * data it reads, or this function won't work.
 * disable_log_forwarding() should be called when the logs are not needed,
 * to improve performance.
 * If logging is enabled and data is not read periodically, the circular
 * queue element buffer will be exhausted and data WILL be lost.
 * @param out: char of data returned from queue.
 */
void dequeue_logger_data(char *out);