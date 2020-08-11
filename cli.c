/**
 * @file cli.c
 * Implements a generic console that can read command strings. Command
 * strings are not handled in this file.
 * Interface-specific (UART, SPI, etc...) handling should be done in
 * another file, this file abstracts it.
 *
 * This code assumes that the connected terminal emulates a VT-100.
 */

/* XDCtools Header files */
#include <xdc/runtime/System.h>
#include <xdc/std.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "commands.h"

#define CLI_EMPTY_LINELEN -1 // Signifies an "unused" history buffer.
#define CLI_PROMPT "-> "

// functions to handle sub cases for the CLI.
static void cli_handle_return(CLIContext *context);
static void cli_handle_esc(CLIContext *context);
static void cli_handle_backspace(CLIContext *context);
// Helper functions
static void move_line_index(CLIContext *context, bool forwards);

/**
 * Initializes memory for a CLI context.
 * @param context: CLI context to init.
 */
void cli_context_init(CLIContext *context) {
    int i;
    context->cursor = NULL;
    context->line_idx = 0;
    for (i = 0; i < CLI_BUFCNT; i++) {
        // Set the line length
        context->lines[i].len = CLI_EMPTY_LINELEN;
    }
}

/**
 * CLI Printf function. Same syntax as printf.
 * @param context CLI context to print to
 * @param format printf style format string
 */
void cli_printf(CLIContext *context, const char *format, ...) {
    va_list args;
    int num_print;
    char output_buf[PRINT_BUFLEN];
    // Init variable args list.
    va_start(args, format);
    // Print to buffer and destroy varargs list.
    /*
     * TODO: ideally, we'd implement printf with a circular buffer, so we can
     * print a string of any length.
     * Note: vsnprintf uses the heap.
     */
    num_print = vsnprintf(output_buf, PRINT_BUFLEN, format, args);
    va_end(args);
    // Write the shorter value between the buffer size and num_print
    context->cli_write(output_buf,
                       num_print > PRINT_BUFLEN ? PRINT_BUFLEN : num_print);
}

/**
 * Runs the embedded CLI for this program. The provided context exposes read
 * and write functions for a communication interface.
 * @param context: CLI context for I/O.
 */
void start_cli(CLIContext *context) {
    CLI_Line *current_line;
    char input;
    /*
     * CLI loop, reads a line of data then handles it according to
     * avaliable targets.
     */
    while (1) {
        // Print prompt.
        context->cli_write(CLI_PROMPT, sizeof(CLI_PROMPT) - 1);
        current_line = &(context->lines[context->line_idx]);
        // Initialize cursor location to start of buffer.
        context->cursor = current_line->line_buf;
        // Zero length of buffer.
        current_line->len = 0;
        /*
         * Set history entry after this one to unused.
         * This is why we need one more entry in history buffer than
         * we actually use.
         */
        move_line_index(context, true);
        context->lines[context->line_idx].len = CLI_EMPTY_LINELEN;
        // Revert change to current line idx.
        move_line_index(context, false);
        // Read data until a LF is found.
        do {
            context->cli_read(&input, 1);
            switch (input) {
            case '\r': // CR, or enter on most consoles.
                cli_handle_return(context);
                break;
            case '\b':
                cli_handle_backspace(context);
                break;
            case '\x1b':
                cli_handle_esc(context);
                break;
            default:
                if (current_line->len >= CLI_MAX_LINE - 1) {
                    break; // Do not echo any more data, nor write to buffer.
                }
                // Simply echo character, and set in buffer
                context->cli_write(&input, 1);
                *(context->cursor) = input;
                // Move to next location in buffer
                (context->cursor)++;
                /*
                 * If we have added to the end of the buffer, increase
                 * line length
                 */
                if (context->cursor - current_line->line_buf >
                    current_line->len) {
                    current_line->len =
                        context->cursor - current_line->line_buf;
                }
                break;
            }
        } while (input != '\r');
    }
}

/**
 * Handles a return character ('\r') on the CLI.
 * Will submit the null terminated command string to a CLI handler function.
 * Following return of handler, will advance CLI history buffer to next entry
 * and return.
 * @param context: CLI context to use for this command.
 */
static void cli_handle_return(CLIContext *context) {
    CLI_Line *current_line = &context->lines[context->line_idx];
    // Write a newline and carriage return to the console.
    context->cli_write("\r\n", 2);
    // Null terminate the command.
    current_line->line_buf[current_line->len] = '\0';
    /**
     * If line is empty, do not advance to next line buffer or
     * handle command.
     * Else, advance to the next index in the line buffer.
     */
    if (current_line->len == 0) {
        current_line->len = CLI_EMPTY_LINELEN;
        return;
    }
    move_line_index(context, true);
    // handle command.
    System_printf("Console Read: %s\n", current_line->line_buf);
    System_flush();
    handle_command(context, current_line->line_buf);
}

/**
 * Handles a backspace ('\b') character on the commandline.
 * If the cursor is at the end of the current statement and there are
 * characters to remove, deletes a character from the screen and buffer, and
 * moves the buffer pointer as well as cursor backwards.
 * @param context: CLI context to use
 */
static void cli_handle_backspace(CLIContext *context) {
    CLI_Line *current_line = &context->lines[context->line_idx];
    /*
     * Backspace: send backspace and space to clear
     * character. Edit the value in the command buffer as well.
     */
    /*
     * Ensure that the cursor is at the end of the line,
     * and there is data to delete.
     */
    if (context->cursor - current_line->line_buf == current_line->len &&
        context->cursor != current_line->line_buf) {
        // This moves the cursor back, writes a space, and moves it again
        // Effectively, this clears the character before the cursor.
        context->cli_write("\b\x20\b", 3);
        // Move cursor back, and nullify the character there.
        context->cursor--;
        *(context->cursor) = '\0';
        // Lower the line length.
        current_line->len--;
    }
}

/**
 * Handles an escape character (typically will resolve to a control code) on
 * the commandline.
 * @param context: Context escape character recieved on.
 */
static void cli_handle_esc(CLIContext *context) {
    CLI_Line *current_line = &context->lines[context->line_idx];
    char esc_buf[2];
    /**
     * Escape sequence. Read more characters from the
     * input to see if we can handle it.
     */
    context->cli_read(esc_buf, sizeof(esc_buf));
    if (esc_buf[0] != '[') {
        /**
         * Not an escape sequence we understand. Print the buffer
         * contents to the terminal and bail.
         */
        context->cli_write(esc_buf, sizeof(esc_buf));
        return;
    }
    /*
     * If the escape sequence starts with '\x1b]',
     * switch on next char.
     */
    switch (esc_buf[1]) {
    case 'A': // Up arrow
        /**
         * If enough history exists, move the CLI commandline
         * to the prior command.
         */
        move_line_index(context, false);
        if (context->lines[context->line_idx].len != CLI_EMPTY_LINELEN) {
            // Update the line buffer and current line.
            current_line = &(context->lines[context->line_idx]);
            // Set cursor to end of line buffer.
            context->cursor = &current_line->line_buf[current_line->len];
            // Clear line of console and write line from history.
            // Clear line, and reset cursor.
            context->cli_write("\x1b[2K\r", 5);
            // Write prompt
            context->cli_write(CLI_PROMPT, sizeof(CLI_PROMPT) - 1);
            context->cli_write(current_line->line_buf, current_line->len);
        } else {
            // Reset current line idx.
            move_line_index(context, true);
        }
        break;
    case 'B': // Down arrow
        /**
         * If enough history exists, move the CLI commandline
         * to the next command.
         */
        move_line_index(context, true);
        if (context->lines[context->line_idx].len != CLI_EMPTY_LINELEN) {
            // Update the line buffer, cursor, and current line.
            current_line = &(context->lines[context->line_idx]);
            // Set cursor to end of line buffer.
            context->cursor = &current_line->line_buf[current_line->len];
            // Clear line of console and write line from history.
            // Clear line, and reset cursor.
            context->cli_write("\x1b[2K\r", 5);
            // Write prompt
            context->cli_write(CLI_PROMPT, sizeof(CLI_PROMPT) - 1);
            context->cli_write(current_line->line_buf, current_line->len);
        } else {
            // Reset current line idx.
            move_line_index(context, false);
        }
        break;
    case 'C': // Right arrow
        if (context->cursor - current_line->line_buf != current_line->len) {
            // Move cursor and print the forward control seq.
            (context->cursor)++;
            context->cli_write("\x1b", 1);
            context->cli_write(esc_buf, sizeof(esc_buf));
        }
        break;
    case 'D': // Left arrow
        if (context->cursor != current_line->line_buf) {
            // Move cursor and print the backward control seq.
            (context->cursor)--;
            context->cli_write("\x1b", 1);
            context->cli_write(esc_buf, sizeof(esc_buf));
        }
        break;
    default:
        // Ignore other escape sequences.
        break;
    }
}

/**
 * Moves the index of the CLI line history buffer.
 * Sets the index only. Does not edit remainder of context.
 * @param context: CLI context to manipulate
 * @param forwards: should the index be moved one buffer forwards or back?
 */
static void move_line_index(CLIContext *context, bool forwards) {
    int new_idx;
    /*
     * This function simulates modulo, since we are aware exactly how far
     * forwards or back we are moving the index, and our modulo is constant.
     */
    if (forwards) {
        new_idx = context->line_idx + 1;
        if (new_idx == CLI_BUFCNT)
            new_idx = 0;
        context->line_idx = new_idx;
    } else {
        new_idx = context->line_idx - 1;
        if (new_idx < 0)
            new_idx = CLI_BUFCNT - 1;
        context->line_idx = new_idx;
    }
}
