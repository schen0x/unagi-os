Make and Install

- binutils-2.40 (2023-01-16)
- gcc-12.2.0 (2022-12-27)
- qemu-8.0.0

```sh
wget -O- https://ftp.gnu.org/gnu/binutils/binutils-2.40.tar.xz | tar xvJ
wget -O- https://ftp.gnu.org/gnu/gcc/gcc-12.2.0/gcc-12.2.0.tar.xz | tar xvJ
wget -O- https://download.qemu.org/qemu-8.0.0.tar.xz | tar xvJ

export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
export WORKINGDIR=$PWD

#### Binutils #### mkdir build-binutils
cd build-binutils
../binutils-2.40*/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install

#### GCC cross compiler ####
# The $PREFIX/bin dir _must_ be in the PATH. We did that above.
which -- $TARGET-as || echo $TARGET-as is not in the PATH
mkdir build-gcc
cd build-gcc
../gcc-12*/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc

#### QEMU ####
mkdir build-qemu8
cd build-qemu8
../configure/qemu-8.* --enable-gtk
make
make install
```

