/**
 * @file sd_card.h
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

#endif