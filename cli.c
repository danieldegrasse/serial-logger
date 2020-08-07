/**
 * @file cli.c
 * Implements console command handling for the SD logger program.
 * Interface-specific handling should be done in another file, this file
 * abstracts it.
 *
 * This code assumes that the connected terminal emulates a VT-100.
 */

#include <string.h>

#include "cli.h"

#define CLI_EMPTY_LINELEN -1 // Signifies an "unused" history buffer.

/**
 * Initializes memory for a CLI context.
 * @param context: CLI context to init.
 */
void cli_context_init(CLIContext *context) {
    int i;
    context->cursor = NULL;
    context->current_line = 0;
    for (i = 0; i < CLI_BUFCNT; i++) {
        // Set the line length
        context->lines[i].len = CLI_EMPTY_LINELEN;
    }
}

/**
 * Runs the embedded CLI for this program. The provided context exposes read
 * and write functions for a communication interface.
 * @param context: CLI context for I/O.
 */
void start_cli(CLIContext *context) {
    char input, esc_buf[2], *line_buf;
    int new_idx;
    CLI_Line *current_line;
    char prompt[] = "-> ";
    /*
     * CLI loop, reads a line of data then handles it according to
     * avaliable targets.
     */
    while (1) {
        // Print prompt.
        context->cli_write(prompt, sizeof(prompt) - 1);
        current_line = &(context->lines[context->current_line]);
        // Initialize cursor location to start of buffer.
        context->cursor = line_buf = current_line->line_buf;
        // Zero length of buffer.
        current_line->len = 0;
        /*
         * Set history entry after this one to unused.
         * This is why we need one more entry in history buffer than
         * we actually use.
         */
        new_idx = context->current_line + 1;
        if (new_idx == CLI_BUFCNT)
            new_idx = 0;
        context->lines[new_idx].len = CLI_EMPTY_LINELEN;
        // Read data until a LF is found.
        do {
            context->cli_read(&input, 1);
            switch (input) {
            case '\r': // CR, or enter on most consoles.
                context->cli_write("\r\n", 2);
                // Null terminate the command.
                line_buf[current_line->len] = '\0';
                /**
                 * If line is empty, do not advance to next line buffer or
                 * handle command.
                 * Else, advance to the next index in the line buffer.
                 */
                if (current_line->len == 0) {
                    current_line->len = CLI_EMPTY_LINELEN;
                    break;
                }
                // simulate modulo here.
                new_idx = context->current_line + 1;
                if (new_idx == CLI_BUFCNT)
                    new_idx = 0;
                context->current_line = new_idx;
                // TODO: handle command.
                context->cli_write("Got Data:", 9);
                context->cli_write(current_line->line_buf, strlen(line_buf));
                context->cli_write("\r\n", 2);
                break;
            case '\b':
                /**
                 * Backspace: send backspace and space to clear
                 * character. Edit the value in the command buffer as well.
                 */
                // Ensure that the cursor is at the end of the line.
                if (context->cursor - line_buf == current_line->len &&
                    context->cursor != line_buf) {
                    context->cli_write("\b\x20\b", 3);
                    // Move cursor back, and clear the character there.
                    context->cursor--;
                    *(context->cursor) = '\0';
                    // Lower the line length.
                    current_line->len--;
                }
                break;
            case '\x1b':
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
                    break;
                }
                /*
                 * If the escape sequence starts with '\x33]',
                 * switch on next char.
                 */
                switch (esc_buf[1]) {
                case 'A': // Up arrow
                    /**
                     * If enough history exists, move the CLI commandline
                     * to the prior command.
                     */
                    // Simulate modulo here, rather than using directly.
                    new_idx = context->current_line - 1;
                    if (new_idx < 0)
                        new_idx = new_idx + CLI_BUFCNT;
                    if (context->lines[new_idx].len != CLI_EMPTY_LINELEN) {
                        // Update the line buffer, cursor, and current line.
                        current_line = &(context->lines[new_idx]);
                        context->current_line = new_idx;
                        context->cursor = line_buf = current_line->line_buf;
                        // Clear line of console and write line from history.
                        // Clear line, and reset cursor.
                        context->cli_write("\x1b[2K\r", 5);
                        // Write prompt
                        context->cli_write(prompt, sizeof(prompt) - 1);
                        context->cli_write(line_buf, current_line->len);
                        break;
                    }
                    break;
                case 'B': // Down arrow
                    /**
                     * If enough history exists, move the CLI commandline
                     * to the next command.
                     */
                    // Simulate modulo here, rather than using directly.
                    new_idx = context->current_line + 1;
                    if (new_idx == CLI_BUFCNT)
                        new_idx = 0;
                    if (context->lines[new_idx].len != CLI_EMPTY_LINELEN) {
                        // Update the line buffer, cursor, and current line.
                        current_line = &(context->lines[new_idx]);
                        context->current_line = new_idx;
                        context->cursor = line_buf = current_line->line_buf;
                        // Clear line of console and write line from history.
                        // Clear line, and reset cursor.
                        context->cli_write("\x1b[2K\r", 5);
                        // Write prompt
                        context->cli_write(prompt, sizeof(prompt) - 1);
                        context->cli_write(line_buf, current_line->len);
                        break;
                    }
                    break;
                case 'C': // Right arrow
                    if (context->cursor - line_buf != current_line->len) {
                        // Move cursor and print the forward control seq.
                        (context->cursor)++;
                        context->cli_write(&input, 1);
                        context->cli_write(esc_buf, sizeof(esc_buf));
                    }
                    break;
                case 'D': // Left arrow
                    if (context->cursor != line_buf) {
                        // Move cursor and print the backward control seq.
                        (context->cursor)--;
                        context->cli_write(&input, 1);
                        context->cli_write(esc_buf, sizeof(esc_buf));
                    }
                default:
                    // Ignore other escape sequences.
                    break;
                }
                break;
            default:
                // Simply echo character, and set in buffer
                context->cli_write(&input, 1);
                *(context->cursor) = input;
                // Move to next location in buffer
                (context->cursor)++;
                /*
                 * If we have added to the end of the buffer, increase
                 * line length
                 */
                if (context->cursor - line_buf > current_line->len) {
                    current_line->len = context->cursor - line_buf;
                }
                break;
            }
        } while (input != '\r' && current_line->len < CLI_MAX_LINE);
    }
}