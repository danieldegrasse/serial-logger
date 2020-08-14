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
 * Statically allocated queue for received UART log data. Used for forward data
 * when requested. See "Queue Creation" in cfg file.
 */
extern Queue_Handle uart_log_queue;
// Declared in uart_logger_task.c
extern bool FORWARD_UART_LOGS;
// Queue element structure.
typedef struct {
    Queue_Elem elem;
    char data;
} UART_Queue_Elem;

/*
 * PreOS Task for UART logger. Sets up uart instance for data transmission,
 * This code MUST be called before the BIOS is started.
 */
void uart_logger_prebios(void);