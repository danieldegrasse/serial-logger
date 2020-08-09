/**
 * @file commands.c
 * Implements CLI command handlers.
 */
#include "cli.h"

typedef struct {
    char *cmd_name;
    int (*cmd_fxn)(CLIContext *, char**, int);
    char *cmd_help;
} CmdEntry;

#include <string.h>

/*
 * Maximum number of arguments that the parser will handle. 
 * Includes command name.
 */
#define MAX_ARGV 8 


static int help(CLIContext *cxt, char **argv, int argc);

/**
 * Declaration of commands. Syntax is as follows:
 * {"NAME_OF_COMMAND", command_function, "HELP_STRING"}
 * The command function follows the signature of "main", but with a 
 * Context Parameter, ex:
 *   int command_function(CLIContext *ctx, char** argv, int argc)
 * A return value of zero indicates success, anything else indicates failure.
 */

const CmdEntry COMMANDS[] = {
    {"help", help, "Prints help for this commandline"},
    // Add more entries here.
    {NULL, NULL, NULL}
};



/**
 * Handles a command, as given by the null terminated string "cmd"
 * @param ctx: CLI context to print to.
 * @param cmd: command string to handle.
 * @return 0 on successful handling, or another value on failure.
 */
int handle_command(CLIContext *ctx, char *cmd) {
    char *arguments[MAX_ARGV];
    // The parser interprets a space as a delimeter between arguments.
    (void)arguments;
    ctx->cli_write("This will work\r\n", 16);
    cli_printf(ctx, "Hello there");
    return 0;
}


/**
 * Help function. Prints avaliable commandline targets.
 * @param argv: list of all string arguments given (first will be "help")
 * @param argc: length of the argv array.
 */
static int help(CLIContext *ctx, char **argv, int argc) {
    CmdEntry *entry;
    if (argc == 1) {
        cli_printf(ctx, "Test\n");
        entry = (CmdEntry*)&COMMANDS[0];
        while (entry->cmd_name != NULL) {
            // Write command name and newline
            cli_printf(ctx, "%s\n", entry->cmd_name);
            entry++;
        }
    }
    return 0;
}