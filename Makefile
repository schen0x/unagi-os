QEMUPATH = /usr/local/bin
#QEMUPATH = /usr/bin
SHELL := /bin/bash
PJHOME = .

#====================[64bit]====================
EDK2PATH = $(HOME)/src/edk2
EDK2UEFIIMGPATH = $(EDK2PATH)/Build/UnagiLoaderX64/DEBUG_GCC5/X64/
TOOLPATH64 = $(PJHOME)/modules/unagios-build/devenv
DISK_IMG = $(PJHOME)/build/disk.img
MNT_POINT = $(PJHOME)/mnt
# llvm libc++
LIBCXX_DIR=$(HOME)/opt/cross64/x86_64-elf
CLANG_CPPFLAGS = -I$(LIBCXX_DIR)/include/c++/v1 -I$(LIBCXX_DIR)/include -I$(LIBCXX_DIR)/include/freetype2 -I$(EDK2PATH)/MdePkg/Include -I$(EDK2PATH)/MdePkg/Include/X64 -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS -DEFIAPI='__attribute__((ms_abi))'
LD_LLDFLAGS = -L$(LIBCXX_DIR)/lib

RUNQEMU64=$(QEMUPATH)/qemu-system-x86_64 -m 1G -drive if=pflash,format=raw,readonly=on,file=$(TOOLPATH64)/OVMF_CODE.fd -drive if=pflash,format=raw,file=$(TOOLPATH64)/OVMF_VARS.fd -drive if=ide,index=0,media=disk,format=raw,file=$(DISK_IMG) -device nec-usb-xhci,id=xhci -device usb-mouse -device usb-kbd -serial stdio -debugcon file:/run/shm/debug.log -global isa-debugcon.iobase=0x402

all64: clean compileuefi64 compilekernel64 makeimg64 run64
allcompile64: clean compileuefi64 compilekernel64 makeimg64
allgdb64: clean compileuefi64 compilekernel64 makeimg64 gdb64

# .op64 for cpp 64-bit object, .o64 for C 64-bit object, .asmo64 for asm 64-bit object
OBJ64 = main.op64
BUILD_DIR=$(PJHOME)/build

compilekernel64: $(OBJ64) Makefile
	ld.lld $(LD_LLDFLAGS) --entry KernelMain -z norelro --image-base 0x100000 --static -o $(BUILD_DIR)/kernel.elf $(patsubst %, $(BUILD_DIR)/%, $(OBJ64))

compileuefi64: $(PJHOME)/UnagiLoaderPkg/efimain.c Makefile
	cd $(EDK2PATH); source $(EDK2PATH)/edksetup.sh && build
run64:
	$(RUNQEMU64)
gdb64:
	$(RUNQEMU64) -S -gdb tcp:127.0.0.1:1234
makeimg64:
	$(QEMUPATH)/qemu-img create -f raw $(DISK_IMG) 200M
	mkfs.fat -n 'Unagi OS' -s 2 -f 2 -R 32 -F 32 $(DISK_IMG)
	mkdir -p $(MNT_POINT)
	sudo mount -o loop $(DISK_IMG) $(MNT_POINT) && sudo mkdir -p $(MNT_POINT)/EFI/BOOT && \
	sudo cp $(EDK2UEFIIMGPATH)/Loader.efi $(MNT_POINT)/EFI/BOOT/BOOTX64.EFI
	sudo cp $(EDK2UEFIIMGPATH)/Loader.efi $(EDK2UEFIIMGPATH)/Loader.debug $(EDK2UEFIIMGPATH)/TOOLS_DEF.X64 $(PJHOME)/build/
	sudo cp $(PJHOME)/build/kernel.elf $(MNT_POINT)/kernel.elf
	sleep 0.5
	sudo umount $(DISK_IMG)

%.op64: $(PJHOME)/src/%.cpp Makefile
	mkdir -p $(BUILD_DIR)/$(dir $@)
	clang++ $(CLANG_CPPFLAGS) -O0 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++17 -c $< -o $(BUILD_DIR)/$@

%.asmo64: $(PJHOME)/src/%.asm Makefile
	mkdir -p $(BUILD_DIR)/$(dir $@)
	nasm -f elf -g $(PJHOME)/$< -o $(BUILD_DIR)/$@

#====================[32bit]====================
SRC32=$(PJHOME)/src
GCC_KERNEL_INCLUDES = -I$(SRC32)
GCC_KERNEL_FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -fvar-tracking -Iinc -O0
TOOLPATH = $(HOME)/opt/cross/bin

# All .c and .asm files;
#! SRC_C := $(wildcard $(PJHOME)/src/*.c) $(wildcard $(PJHOME)/src/**/*.c)
#! SRC_ASM := $(filter-out %boot.asm %boot_next.asm, $(wildcard $(PJHOME)/src/**/*.asm)) $(wildcard $(PJHOME)/src/*.asm)
SRC_C := $(shell find $(SRC32) -name '*.c')
#! SRC_C := $(wildcard *.c)
SRC_ASM := $(filter-out %boot.asm %boot_next.asm, $(shell find $(SRC32) -name '*.asm'))
#! SRC_ASM := $(filter-out %boot.asm %boot_next.asm, $(wildcard *.asm))

#OBJ_C = $(patsubst %.c,%.o,$(SRC_C))
OBJ_C := $(patsubst $(PJHOME)/src/%.c, $(PJHOME)/build/%.o, $(SRC_C))
# OBJ_ASM = $(patsubst %.asm,%.asmo,$(SRC_ASM))
OBJ_ASM := $(patsubst $(PJHOME)/src/%.asm, $(PJHOME)/build/%.asmo, $(SRC_ASM))

all: clean builddir compile32 run32
allcompile: clean builddir compile32
allgdb: clean builddir compile32 gdb32

builddir:
	mkdir -p build/gdt build/idt build/memory build/memory/paging build/util build/io build/pic build/drivers build/disk build/fs ./build/include/uapi ./build/drivers/graphic build/font build/kernel
FILES = ./build/kernel.asmo $(PJHOME)/build/kernel.o $(PJHOME)/build/idt/idt.asmo $(PJHOME)/build/idt/idt.o $(PJHOME)/build/memory/memory.o $(PJHOME)/build/util/kutil.o $(PJHOME)/build/io/io.asmo $(PJHOME)/build/io/io.o $(PJHOME)/build/pic/pic.o $(PJHOME)/build/drivers/keyboard.o $(PJHOME)/build/memory/heap.o $(PJHOME)/build/memory/kheap.o $(PJHOME)/build/memory/paging/paging.o $(PJHOME)/build/memory/paging/paging.asmo $(PJHOME)/build/disk/disk.o $(PJHOME)/build/fs/pathparser.o $(PJHOME)/build/include/uapi/graphic.o $(PJHOME)/build/drivers/graphic/colortextmode.o $(PJHOME)/build/disk/dstream.o $(PJHOME)/build/drivers/graphic/videomode.o $(PJHOME)/build/font/hankaku.o $(PJHOME)/build/util/printf.o $(PJHOME)/build/util/arith64.o $(PJHOME)/build/util/fifo.o $(PJHOME)/build/drivers/ps2kbc.o $(PJHOME)/build/drivers/ps2mouse.o $(PJHOME)/build/test.o $(PJHOME)/build/util/dlist.o $(PJHOME)/build/memory/heapdl.o $(PJHOME)/build/drivers/graphic/sheet.o $(PJHOME)/build/pic/timer.o $(PJHOME)/build/gdt/gdt.asmo $(PJHOME)/build/gdt/gdt.o $(PJHOME)/build/kernel/process.asmo $(PJHOME)/build/kernel/process.o $(PJHOME)/build/kernel/mprocessfifo.o


compile32: ./bin/boot.bin ./bin/kernel.bin ./bin/boot_next.bin
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/boot_next.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=512 count=2048 >> ./bin/os.bin
	mv ./bin/os.bin /run/shm/os.bin
	dd if=/run/shm/os.bin bs=512 count=2048 of=./bin/os.bin

run32:
	$(QEMUPATH)/qemu-system-i386 -hda ./bin/os.bin -vga std
	# $(QEMUPATH)/qemu-system-i386 -hda ./bin/os.bin -vga std -curses

gdb32:
	# $(QEMUPATH)/qemu-system-x86_64 -hda ./bin/os.bin -curses -S -s
	# $(QEMUPATH)/qemu-system-i386 -hda ./bin/os.bin -S -gdb tcp:127.0.0.1:1234 -curses
	$(QEMUPATH)/qemu-system-i386 -hda ./bin/os.bin -S -gdb tcp:127.0.0.1:1234

debugmakefile32: $(OBJ_ASM) $(OBJ_C) $(SRC_C) $(SRC_ASM)
	echo $^

# The bootloader
./bin/boot.bin: ./src/boot/boot.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin
./bin/boot_next.bin: ./src/boot/boot_next.asm
	nasm -f bin ./src/boot/boot_next.asm -o ./bin/boot_next.bin
# The 32 bits kernel, depends on two parts, written in asm and C
./bin/kernel.bin: $(FILES)
	echo $^
#./bin/kernel.bin: $(OBJ_C) $(OBJ_ASM)
	$(TOOLPATH)/i686-elf-ld -g -relocatable $^ -o ./build/kernelfull.o
	# $(TOOLPATH)/i686-elf-gcc -T ./src/linker.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/kernelfull.o
	$(TOOLPATH)/i686-elf-ld -T ./src/linker.ld -o ./bin/kernel.bin -O0 ./build/kernelfull.o

$(PJHOME)/build/%.o: $(PJHOME)/src/%.c
#%.o: %.c
	mkdir -p $(dir $@)
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c $< -o $(PJHOME)/$@

$(PJHOME)/build/%.asmo: $(PJHOME)/src/%.asm
# %.asmo: %.asm
	mkdir -p $(dir $@)
	nasm -f elf -g $(PJHOME)/$< -o $(PJHOME)/$@


clean:
	rm -rf $(PJHOME)/build/*
	rm -rf $(PJHOME)/bin/*
