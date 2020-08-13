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
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>

/* Board header file */
#include "Board.h"

#include "sd_card.h"

// UART configuration.
#define LOG_BAUD_RATE 115200
#define UART_LOGDEV Board_UART3
// Number of bytes to read from the UART before flushing to SD card.
#define READSIZE 16

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
    char read_buf[READSIZE];
    /*
     * Try to mount the SD card, and if it fails wait for the sd_ready
     * condition.
     */
    if (!attempt_sd_mount()) {
        // Wait for the SD card to be ready and mounted.
        wait_sd_ready();
    }
    System_printf("SD card mounted\n");
    System_flush();
    // Now, try to read data from the UART connection.
    while (1) {
        UART_read(uart, read_buf, READSIZE);
        System_printf("Got data %c\n", *read_buf);
        System_flush();
    }
}