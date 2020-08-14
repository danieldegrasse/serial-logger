/**
 * @file uart_logger.h
 * Implements the task responsible for reading data from a UART, and writing it
 * to an SD card log file.
 *
 * Pins Required:
 * PD6- UART RX
 * PD7- UART TX
 */

#ifndef UART_LOGGER_TASK_H
#define UART_LOGGER_TASK_H

#include <stdbool.h>

#include "cli.h"

/*
 * PreOS Task for UART logger. Sets up uart instance for data transmission,
 * This code MUST be called before the BIOS is started.
 */
void uart_logger_prebios(void);

/**
 * Enables UART log forwarding.
 * @param context: CLI context to log to
 * @return 0 if log forwarding was enabled, or -1 if another console is already
 * using the forwarding feature.
 */
int enable_log_forwarding(CLIContext *context);

/**
 * Disables UART log forwarding.  
 * @return 0 if log forwarding could be disabled, or -1 if CLI does not have
 * permissions to do so.
 */
int disable_log_forwarding(void);

#endif