#!/usr/bin/env sh

rm pub sub

# build c
gcc pub.c -lmosquitto -o pub
gcc sub.c -lmosquitto -o sub
