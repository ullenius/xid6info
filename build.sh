#!/bin/sh
set -eu
cc -Wall -Wextra -Wpointer-arith -pedantic -O3 xid6.c -o xid6info
