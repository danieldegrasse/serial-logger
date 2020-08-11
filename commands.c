/**
 * @file commands.c
 * Implements CLI command handlers.
 */

#include <string.h>

#include "cli.h"
#include "sd_card.h"

typedef struct {
    char *cmd_name;
    int (*cmd_fxn)(CLIContext *, char **, int);
    char *cmd_help;
} CmdEntry;

/*
 * Maximum number of arguments that the parser will handle.
 * Includes command name.
 */
#define MAX_ARGC 8
/*
 * Delimiter used for parsing arguments.
 * Add any other delimeters to this string.
 */
#define DELIMETER " "

static int help(CLIContext *ctx, char **argv, int argc);
static int sdcard(CLIContext *ctx, char **argv, int argc);

/**
 * Declaration of commands. Syntax is as follows:
 * {"NAME_OF_COMMAND", command_function, "HELP_STRING"}
 * The command function follows the signature of "main", but with a
 * Context Parameter, ex:
 *   int command_function(CLIContext *ctx, char** argv, int argc)
 * A return value of zero indicates success, anything else indicates failure.
 */

const CmdEntry COMMANDS[] = {
    {"help", help,
     "Prints help for this commandline.\r\n"
     "supply the name of a command after \"help\" for help with that command"},
    {"sdcard", sdcard,
     "Manages the sdcard. Use \"sdcard mount\" to mount the sdcard"},
    // Add more entries here.
    {NULL, NULL, NULL}};

/**
 * Handles a command, as given by the null terminated string "cmd"
 * @param ctx: CLI context to print to.
 * @param cmd: command string to handle.
 * @return 0 on successful handling, or another value on failure.
 */
int handle_command(CLIContext *ctx, char *cmd) {
    const CmdEntry *entry;
    int argc;
    char *arguments[MAX_ARGC], *saveptr, cmd_buf[CLI_MAX_LINE];
    strncpy(cmd_buf, cmd, CLI_MAX_LINE);
    // The parser interprets a space as a delimeter between arguments.
    // Init strtok_r.
    arguments[0] = strtok_r(cmd_buf, DELIMETER, &saveptr);
    // Parse the rest of the arguments.
    for (argc = 1; argc < MAX_ARGC; argc++) {
        arguments[argc] = strtok_r(NULL, DELIMETER, &saveptr);
        if (arguments[argc] == NULL) {
            // Out of args to parse, break.
            break;
        }
    }
    // Now, find the command to run.
    entry = COMMANDS;
    while (entry->cmd_name != NULL) {
        if (strncmp(entry->cmd_name, arguments[0], CLI_MAX_LINE) == 0) {
            return entry->cmd_fxn(ctx, arguments, argc);
        }
        entry++;
    }
    // If we exit the loop, we don't recognize this command.
    cli_printf(ctx, "Warning: unknown command. Try \"help\". \r\n");
    return 0;
}

/**
 * Help function. Prints avaliable commandline targets.
 * @param argv: list of all string arguments given (first will be "help")
 * @param argc: length of the argv array.
 */
static int help(CLIContext *ctx, char **argv, int argc) {
    const CmdEntry *entry = COMMANDS;
    if (argc == 1) {
        cli_printf(ctx, "Avaliable Commands:\r\n");
        while (entry->cmd_name != NULL) {
            // Write command name and newline
            cli_printf(ctx, "%s\r\n", entry->cmd_name);
            entry++;
        }
        return 0;
    } else if (argc == 2) {
        while (entry->cmd_name != NULL) {
            if (strncmp(entry->cmd_name, argv[1], CLI_MAX_LINE) == 0) {
                // Print help for this command.
                cli_printf(ctx, "%s: %s\r\n", entry->cmd_name, entry->cmd_help);
                return 0;
            }
            entry++;
        }
        // If we make it here, the command name was unknown.
        cli_printf(ctx, "Unknown command: %s\r\n", argv[1]);
        return 255;
    }
    // If neither of the above conditions are triggered, print error.
    cli_printf(ctx, "Unsupported number of arguments\r\n");
    return 255;
}

/**
 * SD card handler function. Allows the console to control the state of the
 * SD card by mounting or unmounting it.
 * @param ctx: CLI context to print to
 * @param argv list of arguments
 * @param argc argument count
 * @return 0 on success, or another value on failure
 */
static int sdcard(CLIContext *ctx, char **argv, int argc) {
    if (argc != 2) {
        cli_printf(ctx, "Unsupported number of arguments\r\n");
        return 255;
    }
    if (strncmp(argv[1], "mount", 5) == 0) {
        cli_printf(ctx, "Attempting to mount sdcard...\r\n");
        if (attempt_sd_mount()) {
            cli_printf(ctx, "Success\r\n");
        } else {
            cli_printf(ctx, "Failed.\r\n");
        }
        return 0;
    } else {
        cli_printf(ctx, "Unknown command %s. Try \"help sdcard\"\r\n", argv[1]);
        return 255;
    }
}