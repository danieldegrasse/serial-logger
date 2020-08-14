/**
 * @file uart_logger.c
 * Implements the task responsible for reading data from a UART, and writing it
 * to an SD card log file.
 *
 * Pins Required:
 * PC6- UART RX
 * PC7- UART TX
 */

/* XDCtools Header files */
#include <xdc/runtime/System.h>
#include <xdc/std.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/UART.h>

/* Board header file */
#include "Board.h"

#include "sd_card.h"
#include "uart_logger_task.h"

// UART configuration.
#define LOG_BAUD_RATE 115200
#define UART_LOGDEV Board_UART3

/*
 * Statically allocated semaphore for data being in queue. See "Semaphore
 * Creation" in .cfg file.
 */
extern Semaphore_Handle logger_data_avail_sem;

/*
 * Statically allocated queue for received UART log data. Used for forward data
 * when requested. See "Queue Creation" in cfg file.
 */
extern Queue_Handle uart_log_queue;

// Queue element structure.
typedef struct {
    Queue_Elem elem;
    char data;
} UART_Queue_Elem;

// If set to true, this task will enqueue UART data as it reads it.
bool FORWARD_UART_LOGS = false;
// Maximum number of chars we will enqueue.
#define MAX_QUEUE 64

static UART_Handle uart;
static UART_Params params;

/*
 * PreOS Task for UART logger. Sets up uart instance for data transmission,
 * This code MUST be called before the BIOS is started.
 */
void uart_logger_prebios(void) {
    /*
     * UART defaults to text mode, echo back characters, and return from read
     * after newline. Default 8 bits, one stop bit, no parity.
     */
    UART_Params_init(&params);
    params.baudRate = LOG_BAUD_RATE;
    params.readReturnMode = UART_RETURN_FULL;
    // Do not do text manipulation on the data. CLI will handle this.
    params.readDataMode = UART_DATA_BINARY;
    params.writeDataMode = UART_DATA_BINARY;
    params.readEcho = UART_ECHO_OFF;
    uart = UART_open(UART_LOGDEV, &params);
    if (uart == NULL) {
        System_abort("Error opening the UART device");
    }
    System_printf("Setup UART Logger\n");
    System_flush();
}

/*
 * Task entry for the UART logger. This task is created statically,
 * see the "Task creation" section of the cfg file.
 * @param arg0 unused
 * @param arg1 unused
 */
void uart_logger_task_entry(UArg arg0, UArg arg1) {
    char read_char;
    UART_Queue_Elem queue_elements[MAX_QUEUE];
    int queue_idx = 0;
    /*
     * Try to mount the SD card, and if it fails wait for the sd_ready
     * condition.
     */
    if (!attempt_sd_mount()) {
        // Wait for the SD card to be ready and mounted.
        wait_sd_ready();
    }
    while (1) {
        /**
         * Write to the SD card until it is unmounted
         */
        System_printf("SD card mounted\n");
        System_flush();
        // Now, try to read data from the UART connection.
        while (1) {
            // Read data from the UART.
            UART_read(uart, &read_char, 1);
            if (sd_card_mounted()) {
                // Write data out to the SD card.
                if (write_sd(&read_char, 1) != 1) {
                    System_abort("SD card write error");
                }
                // If requested via bool, enqueue data as well.
                if (FORWARD_UART_LOGS) {
                    // Use the next statically allocated queue element.
                    queue_elements[queue_idx].data = read_char;
                    // Add the element to the queue (atomically).
                    Queue_put(uart_log_queue,
                              &(queue_elements[queue_idx].elem));
                    queue_idx++;
                    // Post to semaphore so any waiting tasks know we have data.
                    Semaphore_post(logger_data_avail_sem);
                    if (queue_idx >= MAX_QUEUE) {
                        // Wrap the queue element index back to 0.
                        queue_idx = 0;
                    }
                }
            } else {
                System_printf("SD card was unmounted\n");
                System_flush();
                // Exit loop.
                break;
            }
        }
        // Wait for SD card to be remounted.
        wait_sd_ready();
    }
}

/**
 * Enables UART log forwarding. After this function is called, get_log_char()
 * will work as expected, since the logger task will be enqueuing data.
 */
void enable_log_forwarding(void) { FORWARD_UART_LOGS = true; }

/**
 * Disables UART log forwarding. After this function is called, get_log_char()
 * will stop working, since data will not longer be enqueued.
 */
void disable_log_forwarding(void) { FORWARD_UART_LOGS = false; }

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
void dequeue_logger_data(char *out) {
    UART_Queue_Elem *elem;
    /*
     * Remove the element atomically
     * (we expect other tasks to call this function)
     */
    elem = Queue_get(uart_log_queue);
    *out = elem->data;
}

/**
 * Checks if the logger queue has data.
 * @return true if data is present, or false otherwise.
 */
bool logger_has_data(void) { return !(Queue_empty(uart_log_queue)); }

/**
 * Waits for data to be ready in the logger.
 * @param timeout: How long to wait for data.
 */
void wait_logger_data(int timeout) {
    // Pend on semaphore here.
    Semaphore_pend(logger_data_avail_sem, timeout);
}