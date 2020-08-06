/**
 * @file cli.c
 * Implements console command handling for the SD logger program.
 * Interface-specific handling should be done in another file, this file
 * abstracts it.
 */

#include <string.h>

#include "cli.h"

/**
 * Initializes memory for a CLI context.
 * @param context: CLI context to init.
 * @param read_fxn: read function.
 * @param write_fxn: write function.
 */
void cli_context_init(CLIContext *context, int (*read_fxn)(char *, int),
                      int (*write_fxn)(char *, int)) {
    context->cursor = NULL;
    memset(context->line_buffer, 0, CLI_MAX_LINE);
    context->cli_read = read_fxn;
    context->cli_write = write_fxn;
}

/**
 * Runs the embedded CLI for this program. The provided context exposes read
 * and write functions for a communication interface.
 * @param context: CLI context for I/O.
 */
void start_cli(CLIContext *context) {
    char input, esc_buf[2], *line_buf;
    char prompt[] = "-> ";
    /*
     * CLI loop, reads a line of data then handles it according to
     * avaliable targets.
     */
    while (1) {
        // Print prompt.
        context->cli_write(prompt, sizeof(prompt));
        // Initialize cursor location to start of buffer.
        context->cursor = line_buf = context->line_buffer;
        // Zero out line buffer.
        memset(line_buf, 0, CLI_MAX_LINE);
        // Read data until a LF is found.
        do {
            context->cli_read(&input, 1);
            switch (input) {
            case '\r': // CR, or enter on most consoles.
                context->cli_write("\r\n", 2);
                // TODO: handle command.
                context->cli_write("Got Data:", 9);
                context->cli_write(line_buf, strlen(line_buf));
                context->cli_write("\r\n", 2);
                break;
            case '\b':
                /**
                 * Backspace: send backspace and space to clear
                 * character. Edit the value in the command buffer as well.
                 */
                if (context->cursor != line_buf) {
                    context->cli_write("\b\x20\b", 3);
                    // Move cursor back, and clear the character there.
                    context->cursor--;
                    *(context->cursor) = '\0';
                }
                break;
            case '\x1b':
                /**
                 * Escape sequence. Read more characters from the
                 * input to see if we can handle it.
                 */
                context->cli_read(esc_buf, 2);
                if (esc_buf[0] != '[') {
                    // Not an escape sequence we understand, ignore it.
                    break;
                }
                /*
                 * If the escape sequence starts with '\x33]',
                 * switch on next char.
                 */
                switch (esc_buf[1]) {
                case 'C': // Right arrow
                    if (*(context->cursor) != '\0') {
                        // Move cursor and print the forward control seq.
                        (context->cursor)++;
                        context->cli_write(&input, 1);
                        context->cli_write(esc_buf, 2);
                    }
                    break;
                case 'D': // Left arrow
                    if (context->cursor != line_buf) {
                        // Move cursor and print the backward control seq.
                        (context->cursor)--;
                        context->cli_write(&input, 1);
                        context->cli_write(esc_buf, 2);
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
                break;
            }
        } while (input != '\r' && context->cursor - line_buf < CLI_MAX_LINE);
    }
}