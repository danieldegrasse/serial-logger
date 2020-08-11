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
 */

#ifndef SD_CARD_H
#define SD_CARD_H

/**
 * Sets up required mutex and condition variables for SD card management.
 * Should be called before BIOS starts.
 */
void sd_pthread_setup(void);

#endif