# LivelyStatsBinGrep

Mediasoup stream stats binary logs parser.

## Design

This is a standalone C file with no build dependencies from mediasoup. However, binlog records structures defined here must be in sync with same structures defined in LivelyBinLogs.hpp

## Workflow

### Build with gcc, for example: gcc -v -o LivelyStatsBinGrep LivelyStatsBinGrep.c

### Meson: (first time setup) meson setup out && meson compile -C out

## Usage

usage: ./sfustatsgrep [ -f <format> ] [ -t <interval ms> ] log_name
    log_name           - full path to the log file to parse or wildcard pattern in case -a is used
    -f <format>        - format:
                         1 = CSV with no headers, comma separated (default)
                         2 = CSV with column headers, tab separated 
    -t <interval>      - optional align timestamps to fixed intervals in milliseconds
