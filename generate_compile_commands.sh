#!/bin/bash
# Generate a compile_commands.json for ccls (Language Server)
make --always-make --dry-run \
| grep -wE 'nasm|gcc|g\+\+' \
| grep -wE '\-c|nasm \-f elf \-g' \
| jq -nR '[inputs|{directory:".", command:., file: match(" [^ ]+$").string[1:]}]' \
> compile_commands.json

sed -i 's/"i686-elf-gcc/"PREFIX=\\"$HOME\/opt\/cross\\"; TARGET=i686-elf; PATH=\\"$PREFIX\/bin:$PATH\\"; i686-elf-gcc/g' compile_commands.json

