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
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>

/* Pthread support */
#include <ti/sysbios/posix/pthread.h>

/* TI-RTOS Header files */
#include <ti/drivers/UART.h>

/* Board header file */
#include "Board.h"

#include "cli.h"
#include "sd_card.h"

// UART configuration.
#define LOG_BAUD_RATE 115200
#define UART_LOGDEV Board_UART3

// Protects access to log forwarding so only one CLI task at a time can use it.
static pthread_mutex_t LOG_FORWARD_MUTEX;
// Protects access to log forwarding related global variables.
static pthread_mutex_t LOG_VAR_MUTEX;
static CLIContext *CONTEXT;
static bool FORWARD_UART_LOGS = false;

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
    // Setup the CLI mutex.
    if (pthread_mutex_init(&LOG_FORWARD_MUTEX, NULL) != 0) {
        System_abort("Failed to create log forwarding mutex\n");
    }
    // Setup the variable mutex.
    if (pthread_mutex_init(&LOG_VAR_MUTEX, NULL) != 0) {
        System_abort("Failed to create log variable mutex\n");
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
    char start_str[] = "\r\n--------UART Logger Boot---------\r\n";
    /*
     * Try to mount the SD card, and if it fails wait for the sd_ready
     * condition.
     */
    if (!attempt_sd_mount()) {
        // Wait for the SD card to be ready and mounted.
        wait_sd_ready();
    }
    // Write boot notification.
    if (write_sd(start_str, sizeof(start_str) - 1) != sizeof(start_str) - 1) {
        System_abort("Could not write start message to SD card");
    }
    while (1) {
        /**
         * Write to the SD card until it is unmounted
         */
        System_printf("SD card mounted\n");
        System_flush();
        // Write a notification to the SD card that the logs just started.
        if (write_timestamp() != 0) {
            System_abort("Could not write timestamp to SD card");
        }
        // Now, try to read data from the UART connection.
        while (1) {
            // Read data from the UART.
            UART_read(uart, &read_char, 1);
            if (sd_card_mounted()) {
                // Write data out to the SD card.
                if (write_sd(&read_char, 1) != 1) {
                    System_abort("SD card write error");
                }
                // Attempt to lock the log forwarding variable mutex.
                if (pthread_mutex_lock(&LOG_VAR_MUTEX) != 0) {
                    System_abort("Could not lock access to log variables");
                }
                // If log forwarding was requested, write to the CLI.
                if (FORWARD_UART_LOGS) {
                    CONTEXT->cli_write(&read_char, 1);
                }
                // Drop the lock on log forwarding vars.
                pthread_mutex_unlock(&LOG_VAR_MUTEX);
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
 * Enables UART log forwarding.
 * @param context: CLI context to log to
 * @return 0 if log forwarding was enabled, or -1 if another console is already
 * using the forwarding feature.
 */
int enable_log_forwarding(CLIContext *context) {
    // First, get the mutex lock required for log forwarding.
    if (pthread_mutex_trylock(&LOG_FORWARD_MUTEX) != 0) {
        // Another thread owns the mutex, return.
        return -1;
    }
    // Now, get the mutex lock required to edit the forwarding variables.
    if (pthread_mutex_lock(&LOG_VAR_MUTEX) != 0) {
        System_abort("Could not lock access to log variables");
    }
    // Now that we own the mutex, enable forwarding and set the CLI context.
    FORWARD_UART_LOGS = true;
    CONTEXT = context;
    // Unlock the log variable mutex.
    pthread_mutex_unlock(&LOG_VAR_MUTEX);
    /*
     * Note: do not drop the Mutex here. The CLI task thread holds the mutex
     * until it stops log forwarding, preventing other CLI tasks from starting
     * log forwarding while it is using it.
     */
    return 0;
}

/**
 * Disables UART log forwarding.  
 * @return 0 if log forwarding could be disabled, or -1 if CLI does not have
 * permissions to do so.
 */
int disable_log_forwarding(void) { 
    // Lock the forwarding variables mutex.
    if (pthread_mutex_lock(&LOG_VAR_MUTEX) != 0) {
        System_abort("Could not lock access to log variables");
    }
    /*
     * Now, try to unlock the log forwarding mutex. If this fails, the calling 
     * thread doesn't have log forwarding running, and shouldn't be disabling
     * it.
     */
    if (pthread_mutex_unlock(&LOG_FORWARD_MUTEX) != 0) { 
        /*
         * Don't edit the log forwarding variables, just drop their mutex 
         * and return.
         */
        pthread_mutex_unlock(&LOG_VAR_MUTEX);
        return -1;
    }
    // If we were able to unlock the mutex, reset the log forwarding variables.
    FORWARD_UART_LOGS = false; 
    CONTEXT = NULL;
    // Now, drop the log variable mutex
    pthread_mutex_unlock(&LOG_VAR_MUTEX);
    return 0;
}

/**
 * Writes to the UART device being logged from.
 * @param data: buffer of data to write
 * @param len: length of data to write
 * @return number of bytes written.
 */
int write_to_logger(char* data, int len) {
    return UART_write(uart, data, len);
}