/**
 * @file uart_log_reader_task.c
 * This file implements a task (and functions to control it) based around
 * reading data from the UART logger, and relaying it to a CLI context.
 */

/* XDCtools Header files */
#include <xdc/runtime/System.h>
#include <xdc/std.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>

/* Pthread support */
#include <ti/sysbios/posix/pthread.h>

#include "cli.h"
#include "uart_logger_task.h"

// Protects access to the CLI context global variable.
static pthread_mutex_t LOG_READER_CTX_MUTEX;
// Current context we will print to.
static CLIContext *CONTEXT;
/**
 * This semaphore is used to signal this task to run. Statically created, see
 * "Semaphore Creation" in .cfg file.
 */
extern Semaphore_Handle log_reader_sem;
// Also statically created. Lets task acknowledge shutdown.
extern Semaphore_Handle shutdown_ack_sem;

/**
 * Must be called before BIOS starts. Sets up required data structures for
 * the UART log reader task.
 */
void uart_log_reader_prebios(void) {
    if (pthread_mutex_init(&LOG_READER_CTX_MUTEX, NULL) != 0) {
        System_abort("Failed to create SD write mutex\n");
    }
    CONTEXT = NULL;
}

/**
 * Starts the log reader with a given CLI context. If another log reader is
 * running, this function will block until it terminates.
 * @param context: CLI context the log reader must print to.
 */
void start_log_reader(CLIContext *context) {
    // First, lock the mutex.
    if (pthread_mutex_lock(&LOG_READER_CTX_MUTEX) != 0) {
        System_abort("could not lock log reader mutex");
    }
    CONTEXT = context;
    /*
     * Now, post to the binary semaphore. The log reader task will see this
     * signal and start writing to the set context.
     */
    Semaphore_post(log_reader_sem);
    /*
     * Note: we purposely do NOT unlock the mutex here. This is how we keep
     * other tasks from starting the log reader with their CLI context.
     */
}

/**
 * Task entry for the UART log reader. Will wait for a semaphore to be
 * signalled, and upon signaling will run until the semaphore is
 * signalled again.
 * The run loop of the log reader will read data from the UART logger, and
 * echo this data to a selected CLI context.
 * @param arg0: Unused
 * @param arg1: Unused
 */
void uart_log_reader_task_entry(UArg arg0, UArg arg1) {
    char uart_data;
    while (1) {
        // Wait for the semaphore to be signaled.
        Semaphore_pend(log_reader_sem, BIOS_WAIT_FOREVER);
        System_printf("Running Log reader\n");
        System_flush();
        cli_printf(CONTEXT, "We are running the task now\r\n");
        // Enable log forwarding here.
        enable_log_forwarding();
        while (1) {
            // If data is avaliable, read it and print to CLI.
            if (logger_has_data()) {
                dequeue_logger_data(&uart_data);
                // Bypass cli_printf here, mostly for speed.
                CONTEXT->cli_write(&uart_data, 1);
            } else {
                // Wait one second for logger data before trying again
                /*
                 * We cannot wait forever because we need to be able to 
                 * shutdown the logger in a timely manner, even if no new data
                 * has arrived from UART.
                 */
                wait_logger_data(1000);
            }
            /*
             * Check to see if the semaphore was signaled again. If so, a task
             * is requesting we exit.
             */
            if (Semaphore_pend(log_reader_sem, BIOS_NO_WAIT) == TRUE) {
                // Task requested exit. Disable log forwarding.
                System_printf("Shutdown of log reader requested\n");
                System_flush();
                disable_log_forwarding();
                // Clear the value of the global context.
                CONTEXT = NULL;
                // Post to shutdown ack sem to acknowledge exit.
                Semaphore_post(shutdown_ack_sem);
                break;
            }
        }
    }
}

/**
 * Stops the log reader. Must be called from the same task context that
 * is currently running the log reader.
 * @return 0 if the log reader stopped, or -1 if the task trying to stop the
 * reader is not the one that started it.
 */
int stop_log_reader(void) {
    if (CONTEXT == NULL) {
        // No CLI context is running, so the log reader isn't either.
        // Return 0, as the log reader is stopped (since it wasn't running)
        return 0;
    }
    // First, signal the log reader semaphore. The log reader task will see it.
    Semaphore_post(log_reader_sem);
    /*
     * Now, wait for the log reader task to post to the semaphore. this
     * handshake ensures the log reader task has stopped.
     */
    Semaphore_pend(shutdown_ack_sem, BIOS_WAIT_FOREVER);
    System_printf("Got handshake signal from log reader task\n");
    System_flush();
    // Finally, drop the mutex.
    if (pthread_mutex_unlock(&LOG_READER_CTX_MUTEX) != 0) {
        /*
         * The calling thread does not own the mutex, and was not the thread
         * that started the log reader task. Post to the semaphore again to
         * start the log reader task back up.
         */
        System_printf("Calling thread did not own the mutex!\n");
        System_flush();
        Semaphore_post(log_reader_sem);
        return -1;
    }
    return 0;
}
