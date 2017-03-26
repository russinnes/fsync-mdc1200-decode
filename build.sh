#!/bin/bash
gcc -o demod demod.c fsync_decode.c mdc_decode.c $(pkg-config --cflags --libs libpulse-simple)

