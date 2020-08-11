/**
 * @file heartbeat_task.c
 * Implements the heartbeat task, which simply flashes a LED on the board.
 */
/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS drivers */
#include <ti/drivers/GPIO.h>

/* Board Header file */
#include "Board.h"

/*
 *  ======== heartBeatFxn ========
 *  Toggle the Board_LED0. The Task_sleep is determined by arg0 which
 *  is configured for the heartBeat Task instance.
 */
void heartBeatFxn(UArg arg0, UArg arg1) {
    while (1) {
        Task_sleep((UInt)arg0);
        GPIO_toggle(Board_LED0);
    }
}