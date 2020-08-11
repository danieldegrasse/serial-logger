/**
 * @file uart_logger.c
 * Implements the task responsible for reading data from a UART, and writing it
 * to an SD card log file.
 */

/* XDCtools Header files */
#include <xdc/runtime/System.h>
#include <xdc/std.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include "sd_card.h"

/*
 * Task entry for the UART logger. This task is created statically,
 * see the "Task creation" section of the cfg file.
 * @param arg0 unused
 * @param arg1 unused
 */
void uart_logger_task_entry(UArg arg0, UArg arg1) {
    System_printf("Attempting sd mount\n");
    System_flush();
    /*
     * Try to mount the SD card, and if it fails wait for the sd_ready
     * condition.
     */
    if (!attempt_sd_mount()) {
        System_printf("Sd mount failed\n");
        System_flush();
        wait_sd_ready();
        System_printf("After wait, sd card was mounted\n");
    } else {
        System_printf("SD card mounted successfully\n");
    }
    System_flush();
}