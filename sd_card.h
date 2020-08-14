/**
 * @file sd_card.h
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

#ifndef SD_CARD_H
#define SD_CARD_H
#include <stdbool.h>

/**
 * Sets up required mutex and condition variables for SD card management.
 * Also enables GPIO pins control required for SD card hotplug.
 * Should be called before BIOS starts.
 */
void sd_setup(void);

/**
 * Attempts to mount the SD card. If the mount succeeds, notifies any tasks
 * waiting for the SD card to be mounted that it has been.
 * @return true if mount succeeds, or false otherwise.
 */
bool attempt_sd_mount(void);

/**
 * Unmounts the SD card.
 */
void unmount_sd_card(void);

/**
 * Waits for the SD card to be mounted.
 */
void wait_sd_ready(void);

/**
 * Gets the mount status of the SD card.
 * @return true if card is mounted, false otherwise.
 */
bool sd_card_mounted(void);

/**
 * Writes data to the SD card.
 * @param data data buffer to write to the SD card.
 * @param n number of bytes to write.
 * @return number of bytes written, or -1 on error.
 */
int write_sd(void *data, int n);

/**
 * Gets the size of the log file in bytes.
 * @return size of file in bytes.
 */
int filesize(void);

#endif