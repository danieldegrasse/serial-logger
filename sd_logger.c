/* XDCtools Header files */
#include <xdc/runtime/System.h>
#include <xdc/std.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>

/* Board Header file */
#include "Board.h"

#include "heartbeat_task.h"
#include "sd_card.h"
#include "uart_console_task.h"
#include "uart_logger_task.h"

/*
 *  ======== main ========
 */
int main(void) {
    /* Call general board init*/
    Board_initGeneral();
    Board_initUART(); // Done here since both the console and logger use it.
    uart_console_prebios();
    // Enable the heartbeat task
    Board_initGPIO();
    heartbeat_prebios();
    uart_logger_prebios();
    // Setup required pthread variables for the SD card.
    sd_setup();
    /* Start BIOS */
    BIOS_start();
    return (0);
}
