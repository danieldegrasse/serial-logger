/* XDCtools Header files */
#include <xdc/runtime/System.h>
#include <xdc/std.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
// #include <ti/drivers/I2C.h>
// #include <ti/drivers/SDSPI.h>
// #include <ti/drivers/SPI.h>
// #include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>
// #include <ti/drivers/WiFi.h>

/* Board Header file */
#include "Board.h"

#include "sd_card.h"
#include "uart_console_task.h"

/*
 *  ======== main ========
 */
int main(void) {
    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    uart_task_prebios();
    // Setup required pthread variables for the SD card.
    sd_setup();
    /* Start BIOS */
    BIOS_start();
    return (0);
}
