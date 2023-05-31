# Unagi OS

- [README-os](./README-os.md)


## ENVIRONMENT

```sh
# EDK2 (UEFI)
git clone https://github.com/tianocore/edk2.git $HOME/src/edk2
git log
## > commit e1f5c6249af08c1df2c6257e4bb6abbf6134318c (edk2-stable202305 ++)

# QEMU
qemu-system-i386 --version
qemu-system-x86_64 --version
## QEMU emulator version 8.0.0
## Copyright (c) 2003-2022 Fabrice Bellard and the QEMU Project developers

# CLANG (cpp && 64-bit OS)
clang --version
clang++ --version
## Ubuntu clang version 14.0.0-1ubuntu1
## Target: x86_64-pc-linux-gnu
## Thread model: posix
## InstalledDir: /usr/bin

# GCC (crosscompiler for 32-bit OS)
$HOME/opt/cross/bin/i686-elf-gcc --version
## i686-elf-gcc (GCC) 12.2.0
## Copyright (C) 2022 Free Software Foundation, Inc.
## This is free software; see the source for copying conditions.  There is NO
## warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# BINUTIL
$HOME/opt/cross/bin/i686-elf-ld --version
## GNU ld (GNU Binutils) 2.40
## Copyright (C) 2023 Free Software Foundation, Inc.
## This program is free software; you may redistribute it under the terms of
## the GNU General Public License version 3 or (at your option) a later version.
## This program has absolutely no warranty.

# BEAR
bear --version
# bear 3.0.18

# CCLS
ccls --version
# Ubuntu ccls version 0.20210330-1
# clang version 12.0.1-19ubuntu3
```

- EDK2 Conf/target.txt

```txt
# ...
ACTIVE_PLATFORM       = UnagiLoaderPkg/UnagiLoaderPkg.dsc
# ...
TARGET                = DEBUG
# ...
TARGET_ARCH           = X64
# ...
TOOL_CHAIN_CONF       = Conf/tools_def.txt
# ...
TOOL_CHAIN_TAG        = CLANGPDB
# ...
BUILD_RULE_CONF = Conf/build_rule.txt
```

