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
 * PF4- SD write activity LED.
 * PA2- See below
 * If hotplugging the SD card is desired, PA2 will be pulled high when the
 * system wants to power the SD card. a MOSFET or BJT will be  required because
 * PA2 cannot supply enough current for the SD card.
 */

/* XDCtools Header files */
#include <xdc/runtime/System.h>
#include <xdc/runtime/Timestamp.h>
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
#include <stdio.h>

// Board header file
#include "Board.h"

// Drive number, as well as macros to convert it to a string
#define DRIVE_NUM 0
#define STR_(n) #n
#define STR(n) STR_(n)

// Global variables.
pthread_cond_t SD_CARD_READY;
pthread_mutex_t SD_CARD_RW_MUTEX;
bool SD_CARD_MOUNTED = false;
SDSPI_Handle SDSPI_HANDLE;
FIL LOGFILE;

static bool sd_online(const char *drive_num, FATFS **fs);
static bool open_file(const char *filename, FIL *outfile);

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
}

/**
 * Attempts to mount the SD card. If the mount succeeds, notifies any tasks
 * waiting for the SD card to be mounted that it has been.
 * @return true if mount succeeds, or false otherwise.
 */
bool attempt_sd_mount(void) {
    SDSPI_Params sdspi_params;
    const char logfilename[] = STR(DRIVE_NUM) ":uart_log.txt";
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
     *
     * The SD Card needs to be aware of the SPI signal before it is powered.
     * If the SPI bus has not been setup, it will not be and the MCU will
     * reset.
     *
     * For this reason, we wait to power up the sd card until after the early
     * init of the bus is done (in main).
     */
    // Power up the SD card
    GPIO_write(Board_SDCARD_VCC, Board_LED_ON);
    /* Try to mount the SD card. */
    SDSPI_Params_init(&sdspi_params);
    SDSPI_HANDLE = SDSPI_open(Board_SDSPI0, DRIVE_NUM, &sdspi_params);
    if (SDSPI_HANDLE == NULL) {
        System_abort("Error starting the SD card\n");
    } else {
        System_printf("SPI Bus for Drive %u started\n", DRIVE_NUM);
    }
    success = SD_CARD_MOUNTED = sd_online(STR(DRIVE_NUM), &(LOGFILE.fs));
    if (success) {
        // Sd card did mount. Open the log file for writing.
        if (!open_file(logfilename, &LOGFILE)) {
            System_abort("SD card is mounted, but cannot write file");
        }
        // Signal waiting tasks that the SD card is ready.
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
    // Flush all pending writes to the SD card, and close the log file.
    f_sync(&LOGFILE);
    f_close(&LOGFILE);
    // Power the SD card VCC back off.
    GPIO_write(Board_SDCARD_VCC, Board_LED_OFF);
    // Undo SPI bus initialization.
    SDSPI_close(SDSPI_HANDLE);
    SD_CARD_MOUNTED = false;
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
 * Writes data to the SD card.
 * @param data data buffer to write to the SD card.
 * @param n number of bytes to write.
 * @return number of bytes written, or -1 on error.
 */
int write_sd(void *data, int n) {
    FRESULT fresult;
    unsigned int bytes_written;
    // First, lock the sd card access mutex.
    if (pthread_mutex_lock(&SD_CARD_RW_MUTEX) != 0) {
        System_abort("could not lock sd card mutex");
    }
    fresult = f_write(&LOGFILE, data, n, &bytes_written);
    // Unlock the mutex
    pthread_mutex_unlock(&SD_CARD_RW_MUTEX);
    if (fresult) {
        return -1;
    } else {
        // Toggle write activity LED.
        GPIO_toggle(Board_WRITE_ACTIVITY_LED);
        return (int)bytes_written;
    }
}

/**
 * Writes a timestamp to the SD card logs
 * @return 0 on success, or another value on error.
 */
int write_timestamp(void) {
    char ts_string_buf[80];
    int num_chars;
    // Print the formatted timestamp into buffer.
    num_chars =
        snprintf(ts_string_buf, 80, "\n-------Log Timestamp: %lu -----------\n",
                 Timestamp_get32());
    return (write_sd(ts_string_buf, num_chars) == num_chars ? 0 : -1);
}

/**
 * Gets the size of the log file in bytes.
 * @return size of file in bytes.
 */
int filesize(void) {
    int filesize;
    // First, lock the sd card access mutex.
    if (pthread_mutex_lock(&SD_CARD_RW_MUTEX) != 0) {
        System_abort("could not lock sd card mutex");
    }
    filesize = f_size(&LOGFILE);
    pthread_mutex_unlock(&SD_CARD_RW_MUTEX);
    return filesize;
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

/**
 * Attempts to open the file given by "filename" using the FIL structure
 * "outfile"
 * @param filename: filename to open, formatted with the drive number
 * @param outfile: output file structure to fill
 * @return true if file is opened successfully, or false otherwise.
 */
static bool open_file(const char *filename, FIL *outfile) {
    FRESULT fresult;
    fresult = f_open(outfile, filename, FA_READ | FA_WRITE);
    if (fresult != FR_OK) {
        System_printf("Creating new file \"%s\"\n", filename);
        fresult = f_open(outfile, filename, FA_CREATE_NEW | FA_READ | FA_WRITE);
        if (fresult != FR_OK) {
            return false;
        }
        System_flush();
    } else {
        // Seek to the end of the file.
        fresult = f_lseek(outfile, f_size(outfile));
        if (fresult != FR_OK) {
            f_close(outfile);
            return false;
        }
    }
    return true;
}