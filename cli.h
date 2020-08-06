/**
 * @file cli.h
 * Implements console command handling for the SD logger program.
 * Interface-specific handling should be done in another file, this file
 * abstracts it.
 */
#ifndef CLI_H
#define CLI_H

/** CLI configuration parameters */
#define CLI_MAX_LINE 80

typedef struct {
    /*! read bytes into the buffer. Returns the number of bytes read. */
    int (*cli_read)(char *, int);
    /*! write data for output on the CLI. Returns number of bytes written. */
    int (*cli_write)(char *, int);
    /*! current pointer location */
    char *cursor;
    /*! line buffer */
    char line_buffer[CLI_MAX_LINE];
} CLIContext;

/**
 * Initializes memory for a CLI context.
 * @param context: CLI context to init.
 * @param read_fxn: read function.
 * @param write_fxn: write function.
 */
void cli_context_init(CLIContext *context, int (*read_fxn)(char *, int),
                      int (*write_fxn)(char *, int));

/**
 * Runs the embedded CLI for this program. The provided context exposes read
 * and write functions for a communication interface.
 * @param context: CLI context for I/O.
 */
void start_cli(CLIContext *context);

#endif