# Serial Logger
## About
This project was built to aid in debugging Linux kernel panics that occurred before the relevant logs could be written to disk, but can be used
in any situation where an always on serial logging device is required.

## Building
This project is based around TI-RTOS v2.16.01.14 and XDCTools v3.32.00.06
It compiles using arm-none-eabi-gcc. I compile using gcc 10.2.0, but any version should work. The variables `CODEGEN_INSTALL_DIR`, `XDC_INSTALL_DIR`, and`TIRTOS_INSTALL_DIR` will need to be adjusted for your environment. Once these are set, simply type `make`, or `make release` to build a version that can run without a debugger.

See `gdb.command` for the debugging commands (some are openocd specific) to load the debug build onto a device

## Required Hardware
This project is built to run on a Tiva C Launchpad. One is required.

This project requires an SD card that can be connected via SPI (see `sdcard.c` for specific pins), and some form of UART connection for logging be available (see `uart_logger_task.c` for specific pins). Using the UART commandline can simply be done via the Launchpads' integrated UART connection.

## Flashing executable
Note: Only a rom built with `make release` can be flashed onto the launchpad and run without a connected debugger (due to GCC semihosting semantics). The produced bin file can be flashed using a tool such as lm4flash.

## Usage
Once all wiring is connected, the system should power up and mount the SD card. No further work should be required to use the core logging feature. If you want to use the commandline, open a serial client like PUTTY on the integrated serial connection to the launchpad (on Linux the device is `/dev/ttyAC0`). Type `help` for a list of commands, or `help [command]` for help with a specific command.