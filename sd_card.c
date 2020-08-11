/**
 * @file sd_card.c
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

/* XDCtools Header files */
#include <xdc/runtime/System.h>
#include <xdc/std.h>

/* Pthread support */
#include <ti/sysbios/posix/pthread.h>

/* TI-RTOS driver files */
#include <ti/drivers/SDSPI.h>

/* SD SPI driver */
#include <ti/mw/fatfs/ff.h>

#include <stdbool.h>

// Board header file
#include "Board.h"

// Drive number, as well as macros to convert it to a string
#define DRIVE_NUM 0
#define STR(n) #n


// Global variables.
pthread_cond_t sd_card_mounted;
pthread_mutex_t sd_card_access_mutex;

static bool sd_online(const char *drive_num, FATFS **fs);

/**
 * Sets up required mutex and condition variables for SD card management.
 * Should be called before BIOS starts.
 */
void sd_pthread_setup(void) {
    pthread_cond_init(&sd_card_mounted, NULL);
    if (pthread_mutex_init(&sd_card_access_mutex, NULL) != 0) {
        System_abort("Failed to create SD write mutex\n");
    }
}

/**
 * Attempts to mount the SD card. If the mount succeeds, notifies any tasks
 * waiting for the SD card to be mounted that it has been.
 */
bool attempt_sd_mount(void) {
    // First, lock the sd card access mutex.
    if (pthread_mutex_lock(&sd_card_access_mutex) != 0) {
        System_abort("could not lock sd card mutex");
    }
    // Set up SPI bus.
    Board_initSDSPI();
    SDSPI_Handle sdspi_handle;
    SDSPI_Params sdspi_params;
    FIL logfile;
    /* Try to mount the SD card. */
    SDSPI_Params_init(&sdspi_params);
    sdspi_handle = SDSPI_open(Board_SDSPI0, DRIVE_NUM, &sdspi_params);
    if (sdspi_handle == NULL) {
        System_abort("Error starting the SD card\n");
    } else {
        System_printf("SPI Bus for Drive %u started\n", DRIVE_NUM);
    }
    if (sd_online(STR(DRIVE_NUM), &(logfile.fs))) {
        // Sd card did mount. Signal waiting tasks, and return true.
        pthread_cond_broadcast(&sd_card_mounted);
        return true;
    } else {
        // Sd card is not mounted. Undo SPI bus initialization.
        SDSPI_close(sdspi_handle);
        return false;
    }
}

/**
 * Checks if the SD card is online by attempting to check the free cluster
 * count.
 * @param drive_num: drive number as a string, use STR(DRIVE_NUM)
 * @param fs: FATFS instance
 */
static bool sd_online(const char *drive_num, FATFS **fs) {
    // Verify the SD card is online by getting the free cluster count.
    FRESULT fresult;
    DWORD free_cluster_count;
    fresult = f_getfree(STR(DRIVE_NUM), &free_cluster_count, fs);
    if (fresult) {
        /*
         * Failed to get the free cluster count.
         * Assume SD card is not connected.
         */
        System_printf("Failed to get free cluster count. Assuming SD card"
                      " is offline\n");
        System_flush();
        return false;
    } else {
        System_printf("SD card is online with %lu free clusters\n",
                      free_cluster_count);
        System_flush();
        return true;
    }
}