#!/bin/sh
set -eu
whereis espctag>/dev/null
espctag -a "$1" && ./xid6info "$1"
