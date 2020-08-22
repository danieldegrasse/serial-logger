#ifndef HEARTBEAT_H
#define HEARTBEAT_H
/**
 * @file heartbeat_task.h
 * Implements the heartbeat task, which simply flashes a LED on the board.
 */

/**
 * Sets up the heart beat task, Enabling the Heartbeat LED to be toggled.
 */
void heartbeat_prebios(void);
#endif