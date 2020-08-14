/**
 * @file uart_log_reader_task.h
 * This file implements a task (and functions to control it) based around
 * reading data from the UART logger, and relaying it to a CLI context.
 */

#ifndef UART_LOG_READER_H
#define UART_LOG_READER_H

#include "cli.h"

/**
 * Must be called before BIOS starts. Sets up required data structures for
 * the UART log reader task.
 */
void uart_log_reader_prebios(void);

/**
 * Starts the log reader with a given CLI context. If another log reader is
 * running, this function will block until it terminates.
 * @param context: CLI context the log reader must print to.
 */
void start_log_reader(CLIContext *context);

/**
 * Stops the log reader. Must be called from the same task context that
 * is currently running the log reader.
 * @return 0 if the log reader stopped, or -1 if the task trying to stop the
 * reader is not the one that started it.
 */
int stop_log_reader(void);

#endif