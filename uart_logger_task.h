/**
 * @file uart_logger.h
 * Implements the task responsible for reading data from a UART, and writing it
 * to an SD card log file.
 * 
 * Pins Required:
 * PD6- UART RX
 * PD7- UART TX
 */


/*
 * PreOS Task for UART logger. Sets up uart instance for data transmission,
 * This code MUST be called before the BIOS is started.
 */
void uart_logger_prebios(void);