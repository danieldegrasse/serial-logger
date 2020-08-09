/**
 * @file commands.h
 * Implements CLI command handlers.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

/**
 * Handles a command, as given by the null terminated string "cmd"
 * @param ctx: CLI context to print to.
 * @param cmd: command string to handle.
 * @return 0 on successful handling, or another value on failure.
 */
int handle_command(CLIContext *ctx, char *cmd);

#endif