/**
 * @file sd_card.c
 * Implements all code required to managed the I/O as well as mounting and
 * unmounting of the SD card.
 *
 * ============== Required Pins ==================:
 * The SD card is driven via SPI. The following pins are used:
 * VCC- PE3 (required for hotplugging SD card)
 * CLK- PB4
 * MISO- PB6
 * MOSI- PB7
 * CS- PA5
 *
 * If hotplugging the SD card is desired, PE3 will be pulled high when the
 * system wants to power the SD card. a MOSFET or BJT will be  required because
 * PE3 cannot supply enough current for the SD card.
 */

/* XDCtools Header files */
#include <xdc/runtime/System.h>
#include <xdc/std.h>

/* Pthread support */
#include <ti/sysbios/posix/pthread.h>

/* BIOS Header files */
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS driver files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SDSPI.h>

/* FatFS driver */
#include <ti/mw/fatfs/ff.h>

#include <stdbool.h>

// Board header file
#include "Board.h"

// Drive number, as well as macros to convert it to a string
#define DRIVE_NUM 0
#define STR(n) #n

// Global variables.
pthread_cond_t SD_CARD_READY;
pthread_mutex_t SD_CARD_RW_MUTEX;
bool SD_CARD_MOUNTED = false;
SDSPI_Handle SDSPI_HANDLE;
bool FIRST_INIT = true;

static bool sd_online(const char *drive_num, FATFS **fs);

/**
 * Runs required setup for the SD card. Should be called before BIOS starts.
 */
void sd_setup(void) {
    pthread_cond_init(&SD_CARD_READY, NULL);
    if (pthread_mutex_init(&SD_CARD_RW_MUTEX, NULL) != 0) {
        System_abort("Failed to create SD write mutex\n");
    }
    // Set up SPI bus.
    Board_initSDSPI();
    // Enable GPIO.
    Board_initGPIO();
}

/**
 * Attempts to mount the SD card. If the mount succeeds, notifies any tasks
 * waiting for the SD card to be mounted that it has been.
 * @return true if mount succeeds, or false otherwise.
 */
bool attempt_sd_mount(void) {
    SDSPI_Params sdspi_params;
    FIL logfile;
    bool success;
    // First, lock the sd card access mutex.
    if (pthread_mutex_lock(&SD_CARD_RW_MUTEX) != 0) {
        System_abort("could not lock sd card mutex");
    }
    if (SD_CARD_MOUNTED) {
        // No need to attempt mounting. Return success.
        pthread_mutex_unlock(&SD_CARD_RW_MUTEX);
        System_printf("Mount requested, but SD card already mounted\n");
        System_flush();
        return SD_CARD_MOUNTED;
    }
    /**
     * The power toggle sequence below is required because the MCU will reset
     * if the SD card is plugged in while power is supplied to the SD card.
     * Therefore, we keep the power to the card disabled until a mount of it
     * is requested. The power off here is precautionary, the SD card should
     * not be powered at this point.
     */
    // Power off the SD card by disabling the VCC pin.
    GPIO_write(Board_SDCARD_VCC, Board_LED_OFF);
    /* Try to mount the SD card. */
    SDSPI_Params_init(&sdspi_params);
    SDSPI_HANDLE = SDSPI_open(Board_SDSPI0, DRIVE_NUM, &sdspi_params);
    if (FIRST_INIT) {
        /**
         * FIRST_INIT tracks if the SPI Bus has been setup for the SD card.
         * 
         * The SD Card needs to be aware of the SPI signal before it is powered.
         * If the SPI bus has not been setup, it will not be and the MCU will
         * reset.
         * 
         * Delay for 250ms to let the SD card see the bus.
         */
        Task_sleep(250);
    }
    // Now power up the SD card again.
    GPIO_write(Board_SDCARD_VCC, Board_LED_ON);
    if (SDSPI_HANDLE == NULL) {
        System_abort("Error starting the SD card\n");
    } else {
        System_printf("SPI Bus for Drive %u started\n", DRIVE_NUM);
    }
    success = SD_CARD_MOUNTED = sd_online(STR(DRIVE_NUM), &(logfile.fs));
    if (success) {
        // Sd card did mount. Signal waiting tasks.
        pthread_cond_broadcast(&SD_CARD_READY);
    } else {
        // Sd card is not mounted. Undo SPI bus initialization.
        SDSPI_close(SDSPI_HANDLE);
        // Power the SD card VCC back off.
        GPIO_write(Board_SDCARD_VCC, Board_LED_OFF);
    }
    // Unlock the mutex.
    pthread_mutex_unlock(&SD_CARD_RW_MUTEX);
    System_flush();
    return success;
}

/**
 * Unmounts the SD card.
 */
void unmount_sd_card(void) {
// First, lock the sd card access mutex.
    if (pthread_mutex_lock(&SD_CARD_RW_MUTEX) != 0) {
        System_abort("could not lock sd card mutex");
    }
    // Undo SPI bus initialization.
    SDSPI_close(SDSPI_HANDLE);
    SD_CARD_MOUNTED = false;
    // Power the SD card VCC back off.
    GPIO_write(Board_SDCARD_VCC, Board_LED_OFF);
    // Unlock the mutex.
    pthread_mutex_unlock(&SD_CARD_RW_MUTEX);
}


/**
 * Gets the mount status of the SD card.
 * @return true if card is mounted, false otherwise.
 */
bool sd_card_mounted(void) {
    bool mounted;
    // First, lock the sd card access mutex.
    if (pthread_mutex_lock(&SD_CARD_RW_MUTEX) != 0) {
        System_abort("could not lock sd card mutex");
    }
    mounted = SD_CARD_MOUNTED;
    // Unlock the mutex.
    pthread_mutex_unlock(&SD_CARD_RW_MUTEX);
    return mounted;
}

/**
 * Waits for the SD card to be mounted.
 */
void wait_sd_ready(void) {
    // First, lock the sd card access mutex.
    if (pthread_mutex_lock(&SD_CARD_RW_MUTEX) != 0) {
        System_abort("could not lock sd card mutex");
    }
    pthread_cond_wait(&SD_CARD_READY, &SD_CARD_RW_MUTEX);
    // Unlock the mutex.
    pthread_mutex_unlock(&SD_CARD_RW_MUTEX);
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