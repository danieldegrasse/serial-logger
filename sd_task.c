/**
 * @file sd_task.c
 * Implements all code required to managed the I/O as well as mounting and
 * unmounting of the SD card.
 *
 * ============== Required Pins ==================:
 * The SD card is driven via SPI. The following pins are used:
 * CLK- PB4
 * MISO- PB6
 * MOSI- PB7
 * CS- PA5
 */

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/*
 * Task entry for the SD card task. This task is created statically,
 * see the "Task creation" section of the cfg file.
 * @param arg0 unused
 * @param arg1 unused
 */
void sd_task_entry(UArg arg0, UArg arg1) {

}