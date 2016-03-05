#!/usr/bin/env sh

gcc -Wall -g -I/usr/local/include/modbus client.c -lmodbus -o client