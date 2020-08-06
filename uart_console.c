/*
 * @file uart_console.c
 * Implements a UART console on the built in UART device, allowing for
 * a user to view and manipulate the state of the logger.
 */

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>

/* Board-specific functions */
#include "Board.h"

#include "cli.h"

// UART configuration.
#define BAUD_RATE 115200
#define UART_DEV Board_UART0

static UART_Handle uart;
static UART_Params params;

static int uart_read(char* in, int n);
static int uart_write(char* out, int n);

/*
 * PreOS Task for UART console. Sets up uart instance for data transmission,
 * and creates UART console task.
 * UART device on linux will be ttyACM0.
 * This code MUST be called before the BIOS is started.
 */
void uart_prebios(void)
{
    Board_initUART();
    System_printf("Setup UART Device\n");
    System_flush();
    /*
     * UART defaults to text mode, echo back characters, and return from read 
     * after newline. Default 8 bits, one stop bit, no parity.
     */
    UART_Params_init(&params);
    params.baudRate = BAUD_RATE;
    params.readReturnMode = UART_RETURN_FULL;
    // Do not do text manipulation on the data. CLI will handle this.
    params.readDataMode = UART_DATA_BINARY;
    params.writeDataMode = UART_DATA_BINARY;
    params.readEcho = UART_ECHO_OFF;
    uart = UART_open(UART_DEV, &params);
    if (uart == NULL) {
        System_abort("Error opening the UART device");
    }
}


/*
 * Task entry for the UART console handler. This task is created statically,
 * see the "Task creation" section of the cfg file.
 * @param arg0 unused
 * @param arg1 unused
 */
void uart_task_entry(UArg arg0, UArg arg1)
{
    CLIContext uart_context;
    cli_context_init(&uart_context, uart_read, uart_write);
    start_cli(&uart_context); // Does not return.
}


/**
 * Write data to the UART device.
 * @param out buffer of data to write to UART device
 * @param n length of out in bytes.
 * @return number of byte written to UART.
 */
static int uart_write(char* out, int n) {
    return UART_write(uart, out, n);
}

/**
 * Read data from the UART device.
 * @param in buffer to read data into. Must be "n" bytes or larger.
 * @param n number of bytes to read.
 * @return number of bytes read.
 */
static int uart_read(char* in, int n) {
    return UART_read(uart, in, n);
}