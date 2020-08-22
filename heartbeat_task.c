/**
 * @file heartbeat_task.c
 * Implements the heartbeat task, which simply flashes a LED on the board.
 */
/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS drivers */
#include <ti/drivers/GPIO.h>

#include <stdbool.h>

/* Board Header file */
#include "Board.h"

bool LED_ENABLED = false;

static void button_pressed(unsigned int index) {
    // Toggle the LED enabled state, and disable the LED.
    LED_ENABLED ^= true;
    GPIO_toggle(Board_LED0);
}

/**
 * Sets up the heart beat task, Enabling the Heartbeat LED to be toggled.
 */
void heartbeat_prebios(void) {
    // Install button callback.
    GPIO_setCallback(Board_BUTTON1, button_pressed);
    // Enable button interrupt.
    GPIO_enableInt(Board_BUTTON1);
}

/*
 *  ======== heartBeatFxn ========
 *  Toggle the Board_LED0. The Task_sleep is determined by arg0 which
 *  is configured for the heartBeat Task instance.
 */
void heartBeatFxn(UArg arg0, UArg arg1) {
    LED_ENABLED = true;
    while (1) {
        Task_sleep((UInt)arg0);
        if (LED_ENABLED) {
            GPIO_toggle(Board_LED0);
        }
        /*
         * Turn off the SD write LED. This way if the UART logger hasn't 
         * written data in a while, the LED will be off.
         */
        GPIO_write(Board_WRITE_ACTIVITY_LED, Board_LED_OFF);
    }
}