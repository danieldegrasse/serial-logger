/**
 * @file commands.c
 * Implements CLI command handlers.
 */

#include <string.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>

#include "cli.h"
#include "sd_card.h"
#include "uart_logger_task.h"

/* Board-specific functions */
#include "Board.h"

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
static int mount(CLIContext *ctx, char **argv, int argc);
static int unmount(CLIContext *ctx, char **argv, int argc);
static int sdstatus(CLIContext *ctx, char **argv, int argc);
static int sdpwr(CLIContext *ctx, char **argv, int argc);
static int sdwrite(CLIContext *ctx, char **argv, int argc);
static int logfile_size(CLIContext *ctx, char **argv, int argc);
static int connect_log(CLIContext *ctx, char **argv, int argc);
static int disconnect_log(CLIContext *ctx, char **argv, int argc);

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
    {"mount", mount, "Mounts the SD card"},
    {"unmount", unmount, "Unmounts the SD card"},
    {"sdstatus", sdstatus, "Gets the mount and power status of the SD card"},
    {"sdpwr", sdpwr,
     "Sets the power status of SD card: \"sdpwr on\" or \"sdpwr off\""},
    {"sdwrite", sdwrite, "Writes provided string to the SD card"},
    {"filesize", logfile_size, "Gets the size of the log file in bytes"},
    {"connect_log", connect_log, "Connects to the UART console being logged"},
    {"disconnect_log", disconnect_log,
     "Disconnects from the UART console being logged"},
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
 * Attempts to mount the SD card.
 * @param ctx: CLI context to print to
 * @param argv list of arguments
 * @param argc argument count
 * @return 0 on success, or another value on failure
 */
static int mount(CLIContext *ctx, char **argv, int argc) {
    if (argc != 1) {
        cli_printf(ctx, "Unexpected arguments!\r\n");
        return 255;
    }
    if (sd_card_mounted()) {
        cli_printf(ctx, "SD card is already mounted\r\n");
        return 0;
    }
    cli_printf(ctx, "Attempting to mount sdcard...");
    if (attempt_sd_mount()) {
        cli_printf(ctx, "Success\r\n");
        return 0;
    } else {
        cli_printf(ctx, "Failed\r\n");
        return 255;
    }
}

/**
 * Attempts to unmount the SD card.
 * @param ctx: CLI context to print to
 * @param argv list of arguments
 * @param argc argument count
 * @return 0 on success, or another value on failure
 */
static int unmount(CLIContext *ctx, char **argv, int argc) {
    if (!sd_card_mounted()) {
        cli_printf(ctx, "SD card is not mounted\r\n");
        return 0;
    }
    unmount_sd_card();
    cli_printf(ctx, "SD card unmounted\r\n");
    return 0;
}

/**
 * Reports the mount and power status of the SD card.
 * @param ctx: CLI context to print to
 * @param argv list of arguments
 * @param argc argument count
 * @return 0 on success, or another value on failure
 */

static int sdstatus(CLIContext *ctx, char **argv, int argc) {
    cli_printf(ctx, "SD card is %s\r\n",
               sd_card_mounted() ? "mounted" : "unmounted");
    cli_printf(ctx, "SD card power: %s\r\n",
               GPIO_read(Board_SDCARD_VCC) ? "on" : "off");
    return 0;
}

/**
 * Manually controls power to the SD card. Useful for debugging.
 * @param ctx: CLI context to print to
 * @param argv list of arguments
 * @param argc argument count
 * @return 0 on success, or another value on failure
 */
static int sdpwr(CLIContext *ctx, char **argv, int argc) {
    if (argc != 2) {
        cli_printf(ctx, "Unsupported number of arguments\r\n");
        return 255;
    }
    if (strncmp("on", argv[1], 2) == 0) {
        GPIO_write(Board_SDCARD_VCC, Board_LED_ON);
        cli_printf(ctx, "SD card power on\r\n");
        return 0;
    } else if (strncmp("off", argv[1], 3) == 0) {
        GPIO_write(Board_SDCARD_VCC, Board_LED_OFF);
        cli_printf(ctx, "SD card power off\r\n");
        return 0;
    } else {
        cli_printf(ctx, "Unknown argument %s\r\n", argv[1]);
        return 255;
    }
}

/**
 * Writes provided string to the SD card.
 * @param ctx: CLI context to print to
 * @param argv list of arguments
 * @param argc argument count
 * @return 0 on success, or another value on failure
 */
static int sdwrite(CLIContext *ctx, char **argv, int argc) {
    if (argc != 2) {
        cli_printf(ctx, "Unsupported number of arguments\r\n");
        return 255;
    }
    // Check SD card mount status
    if (!sd_card_mounted()) {
        cli_printf(ctx, "Cannot write to SD card, not mounted\r\n");
        return 255;
    }
    // Write the string to the SD card.
    if (write_sd(argv[1], strlen(argv[1])) != strlen(argv[1])) {
        cli_printf(ctx, "Write error!\r\n");
        return 255;
    }
    return 0;
}

/**
 * Gets the size of the the logfile in bytes.
 * @param ctx: CLI context to print to
 * @param argv list of arguments
 * @param argc argument count
 * @return 0 on success, or another value on failure
 */
static int logfile_size(CLIContext *ctx, char **argv, int argc) {
    cli_printf(ctx, "SD card file size is: %d\r\n", filesize());
    return 0;
}

/**
 * Connects directly to the UART device being logged from. Useful for
 * situations where the logged device exposes a terminal, and you'd like to
 * access it.
 * @param ctx: CLI context to print to
 * @param argv list of arguments
 * @param argc argument count
 * @return 0 on success, or another value on failure
 */
static int connect_log(CLIContext *ctx, char **argv, int argc) {
    if (enable_log_forwarding(ctx) != 0) {
        cli_printf(ctx, "Could not enable log forwarding\r\n");
        return 255;
    } else {
        return 0;
    }
}

/**
 * Disconnects from the UART device being logged, so it's data no longer will
 * print to the CLI.
 * @param ctx: CLI context to print to
 * @param argv list of arguments
 * @param argc argument count
 * @return 0 on success, or another value on failure
 */
static int disconnect_log(CLIContext *ctx, char **argv, int argc) {
    if (disable_log_forwarding() != 0) {
        cli_printf(ctx,
                   "Could not disable log forwarding from this terminal\r\n");
        return 255;
    } else {
        return 0;
    }
}