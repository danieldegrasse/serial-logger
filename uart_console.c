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

// UART configuration.
#define BAUD_RATE 115200
#define UART_DEV Board_UART0

UART_Handle uart;
UART_Params params;

/*
 * PreOS Task for UART console. Sets up uart instance for data transmission,
 * and creates UART console task.
 * UART device on linux will be ttyACM0.
 * This code MUST be called before the BIOS is started.
 */
void uart_prebios(void)
{
    Board_initGPIO();
    Board_initUART();
    params.readDataMode = UART_DATA_BINARY;
    params.writeDataMode = UART_DATA_BINARY;
    // Block reads until newline received.
    params.readReturnMode = UART_RETURN_NEWLINE;
    // Echo all read characters back
    params.readEcho = UART_ECHO_ON;
    params.baudRate = BAUD_RATE;
    uart = UART_open(UART_DEV, &params);
    if (uart == NULL) {
        System_abort("Error opening the UART device");
    }
    System_printf("Setup UART Device\n");
    System_flush();
}


/*
 * Task entry for the UART console handler. This task is created statically,
 * see the "Task creation" section of the cfg file.
 * @param arg0 unused
 * @param arg1 unused
 */
void uartTaskEntry(UArg arg0, UArg arg1)
{
    System_printf("UART task starting\n");
    System_flush();
}

