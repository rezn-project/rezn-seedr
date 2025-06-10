# Building

## Ledger

Ledger is written in C. You can build it on Ubuntu by running

gcc src/ledger.c -o ledger $(pkg-config --cflags --libs notcurses)
