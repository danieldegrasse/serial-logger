/**
 * @file cli.h
 * Implements console command handling for the SD logger program.
 * Interface-specific handling should be done in another file, this file
 * abstracts it.
 */
#ifndef CLI_H
#define CLI_H

/** CLI configuration parameters */
#define CLI_MAX_LINE 80 // max command length
#define CLI_HISTORY 3 // max number of past commands to store

#define CLI_BUFCNT CLI_HISTORY + 2 // Used internally for CLI buffer length

typedef struct {
    /*! buffer to store the commandline data */
    char line_buf[CLI_MAX_LINE];
    /*! length of line */
    int len;
} CLI_Line;

typedef struct {
    /*! read bytes into the buffer. Returns the number of bytes read. */
    int (*cli_read)(char *, int);
    /*! write data for output on the CLI. Returns number of bytes written. */
    int (*cli_write)(char *, int);
    /*! current pointer location */
    char *cursor;
    /*! line buffers */
    CLI_Line lines[CLI_BUFCNT];
    /*! Index of current line buffer */
    int current_line;
} CLIContext;

/**
 * Initializes memory for a CLI context.
 * @param context: CLI context to init.
 */
void cli_context_init(CLIContext *context);

/**
 * Runs the embedded CLI for this program. The provided context exposes read
 * and write functions for a communication interface.
 * @param context: CLI context for I/O.
 */
void start_cli(CLIContext *context);

#endif