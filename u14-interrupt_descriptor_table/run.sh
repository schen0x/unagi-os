#!/bin/bash
# use the cross-compiler environment
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
[[ -z $1 ]] && make all || make allgdb
