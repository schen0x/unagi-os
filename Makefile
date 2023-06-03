FILES = ./build/kernel.asm.o ./build/kernel.o ./build/idt/idt.asm.o ./build/idt/idt.o ./build/memory/memory.o ./build/util/kutil.o ./build/io/io.asm.o ./build/io/io.o ./build/pic/pic.asm.o ./build/pic/pic.o ./build/drivers/keyboard.o ./build/memory/heap.o ./build/memory/kheap.o ./build/memory/paging/paging.o ./build/memory/paging/paging.asm.o ./build/disk/disk.o ./build/fs/pathparser.o ./build/include/uapi/graphic.o ./build/drivers/graphic/colortextmode.o ./build/disk/dstream.o ./build/drivers/graphic/videomode.o ./build/font/hankaku.o ./build/util/printf.o ./build/util/arith64.o ./build/util/fifo.o ./build/drivers/ps2kbc.o ./build/drivers/ps2mouse.o ./build/test.o ./build/util/dlist.o ./build/memory/heapdl.o ./build/drivers/graphic/sheet.o ./build/pic/timer.o ./build/gdt/gdt.asm.o ./build/gdt/gdt.o ./build/kernel/process.asm.o ./build/kernel/process.o ./build/kernel/mprocessfifo.o ./build/main.o
GCC_KERNEL_INCLUDES = -I./src -I$(HOME)/src/edk2/MdePkg/Include
GCC_KERNEL_FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -fvar-tracking -Iinc -O0
TOOLPATH = $(HOME)/opt/cross/bin
QEMUPATH = /usr/local/bin
SHELL := /bin/bash
# 64 bit
PJHOME = .
EDK2PATH = $(HOME)/src/edk2
TOOLPATH64 = $(PJHOME)/modules/unagios-build/devenv
DISK_IMG = $(PJHOME)/build/disk.img
MNT_POINT = $(PJHOME)/mnt

all64: clean builddir compile64 compilekernel64 makeimg64 run64
allcompile64: clean builddir compile
allgdb64: clean builddir compile makeimg gdb

# llvm libc++
LIBCXX_DIR=$(HOME)/opt/cross64/x86_64-elf

CLANG_CPPFLAGS = -I$(LIBCXX_DIR)/include/c++/v1 -I$(LIBCXX_DIR)/include -I$(LIBCXX_DIR)/include/freetype2 -I$(EDK2PATH)/MdePkg/Include -I$(EDK2PATH)/MdePkg/Include/X64 -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS -DEFIAPI='__attribute__((ms_abi))'
LD_LLDFLAGS = -L$(LIBCXX_DIR)/lib

# 32 bit

all: clean builddir compile run
allcompile: clean builddir compile
allgdb: clean builddir compile gdb
allgui: clean builddir compile rungui

builddir:
	mkdir -p build/gdt build/idt build/memory build/memory/paging build/util build/io build/pic build/drivers build/disk build/fs ./build/include/uapi ./build/drivers/graphic build/font build/kernel

compile: ./bin/boot.bin ./bin/kernel.bin ./bin/boot_next.bin
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/boot_next.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=512 count=2048 >> ./bin/os.bin
	mv ./bin/os.bin /run/shm/os.bin
	dd if=/run/shm/os.bin bs=512 count=2048 of=./bin/os.bin
	#dd if=./bin/os.bin bs=512 count=2048 of=/data/VirtualBox_VMs/os0/os0/os0.vdi

run:
	$(QEMUPATH)/qemu-system-i386 -hda ./bin/os.bin -vga std
	# $(QEMUPATH)/qemu-system-i386 -hda ./bin/os.bin -vga std -curses

rungui:
	$(QEMUPATH)/qemu-system-i386 -hda ./bin/os.bin -vga std -usbdevice tablet

gdb:
	# $(QEMUPATH)/qemu-system-x86_64 -hda ./bin/os.bin -curses -S -s
	# $(QEMUPATH)/qemu-system-i386 -hda ./bin/os.bin -S -gdb tcp:127.0.0.1:1234 -curses
	$(QEMUPATH)/qemu-system-i386 -hda ./bin/os.bin -S -gdb tcp:127.0.0.1:1234

# The bootloader
./bin/boot.bin: ./src/boot/boot.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin
./bin/boot_next.bin: ./src/boot/boot_next.asm
	nasm -f bin ./src/boot/boot_next.asm -o ./bin/boot_next.bin
# The 32 bits kernel, depends on two parts, written in asm and C
./bin/kernel.bin: $(FILES)
	$(TOOLPATH)/i686-elf-ld -g -relocatable $(FILES) -o ./build/kernelfull.o
	# $(TOOLPATH)/i686-elf-gcc -T ./src/linker.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/kernelfull.o
	$(TOOLPATH)/i686-elf-ld -T ./src/linker.ld -o ./bin/kernel.bin -O0 ./build/kernelfull.o
./build/kernel.asm.o: ./src/kernel.asm
	nasm -f elf -g ./src/kernel.asm -o ./build/kernel.asm.o
./build/kernel.o: ./src/kernel.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/kernel.c -o ./build/kernel.o
./build/idt/idt.asm.o: ./src/idt/idt.asm
	nasm -f elf -g ./src/idt/idt.asm -o ./build/idt/idt.asm.o
./build/idt/idt.o: ./src/idt/idt.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/idt/idt.c -o ./build/idt/idt.o
./build/memory/memory.o: ./src/memory/memory.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/memory/memory.c -o ./build/memory/memory.o
./build/util/kutil.o: ./src/util/kutil.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/util/kutil.c -o ./build/util/kutil.o
# ./build/gdt/gdt.asm.o: ./src/gdt/gdt.asm
	# nasm -f elf -g ./src/gdt/gdt.asm -o ./build/gdt/gdt.asm.o
# ./build/gdt/gdt.o: ./src/gdt/gdt.c
	# $(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/gdt/gdt.c -o ./build/gdt/gdt.o
./build/io/io.asm.o: ./src/io/io.asm
	# nasm -f elf -g ./src/io/io.asm -o ./build/io/io.asm.elf
	#$(TOOLPATH)/i686-elf-ld -m elf_i386 -o ./build/io/io.asm.o ./build/io/io.asm.elf
	nasm -f elf -F dwarf -g ./src/io/io.asm -o ./build/io/io.asm.o
./build/io/io.o: ./src/io/io.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/io/io.c -o ./build/io/io.o
./build/pic/pic.asm.o: ./src/pic/pic.asm
	nasm -f elf -F dwarf -g ./src/pic/pic.asm -o ./build/pic/pic.asm.o
./build/pic/pic.o: ./src/pic/pic.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/pic/pic.c -o ./build/pic/pic.o
./build/drivers/keyboard.o: ./src/drivers/keyboard.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/drivers/keyboard.c -o ./build/drivers/keyboard.o
./build/memory/heap.o: ./src/memory/heap.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/memory/heap.c -o ./build/memory/heap.o
./build/memory/kheap.o: ./src/memory/kheap.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/memory/kheap.c -o ./build/memory/kheap.o
./build/memory/paging/paging.o: ./src/memory/paging/paging.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/memory/paging/paging.c -o ./build/memory/paging/paging.o
./build/memory/paging/paging.asm.o: ./src/memory/paging/paging.asm
	nasm -f elf -F dwarf -g ./src/memory/paging/paging.asm -o ./build/memory/paging/paging.asm.o
./build/util/printf.o: ./src/util/printf.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/util/printf.c -o ./build/util/printf.o
./build/disk/disk.o: ./src/disk/disk.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/disk/disk.c -o ./build/disk/disk.o
./build/fs/pathparser.o: ./src/fs/pathparser.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/fs/pathparser.c -o ./build/fs/pathparser.o
./build/include/uapi/graphic.o: ./src/include/uapi/graphic.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/include/uapi/graphic.c -o ./build/include/uapi/graphic.o
./build/drivers/graphic/colortextmode.o: ./src/drivers/graphic/colortextmode.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/drivers/graphic/colortextmode.c -o ./build/drivers/graphic/colortextmode.o
./build/disk/dstream.o: ./src/disk/dstream.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/disk/dstream.c -o ./build/disk/dstream.o
./build/drivers/graphic/videomode.o: ./src/drivers/graphic/videomode.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/drivers/graphic/videomode.c -o ./build/drivers/graphic/videomode.o
./build/font/hankaku.o: ./src/font/hankaku.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/font/hankaku.c -o ./build/font/hankaku.o
./build/util/arith64.o: ./src/util/arith64.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/util/arith64.c -o ./build/util/arith64.o
./build/util/fifo.o: ./src/util/fifo.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/util/fifo.c -o ./build/util/fifo.o
./build/drivers/ps2kbc.o: ./src/drivers/ps2kbc.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/drivers/ps2kbc.c -o ./build/drivers/ps2kbc.o
./build/drivers/ps2mouse.o: ./src/drivers/ps2mouse.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/drivers/ps2mouse.c -o ./build/drivers/ps2mouse.o
./build/test.o: ./src/test.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/test.c -o ./build/test.o
./build/util/dlist.o: ./src/util/dlist.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/util/dlist.c -o ./build/util/dlist.o
./build/memory/heapdl.o: ./src/memory/heapdl.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/memory/heapdl.c -o ./build/memory/heapdl.o
./build/drivers/graphic/sheet.o: ./src/drivers/graphic/sheet.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/drivers/graphic/sheet.c -o ./build/drivers/graphic/sheet.o
./build/pic/timer.o: ./src/pic/timer.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/pic/timer.c -o ./build/pic/timer.o
./build/kernel/process.asm.o: ./src/kernel/process.asm
	nasm -f elf -g ./src/kernel/process.asm -o ./build/kernel/process.asm.o
./build/kernel/process.o: ./src/kernel/process.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/kernel/process.c -o ./build/kernel/process.o
./build/gdt/gdt.asm.o: ./src/gdt/gdt.asm
	nasm -f elf -g ./src/gdt/gdt.asm -o ./build/gdt/gdt.asm.o
./build/gdt/gdt.o: ./src/gdt/gdt.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/gdt/gdt.c -o ./build/gdt/gdt.o
./build/kernel/mprocessfifo.o: ./src/kernel/mprocessfifo.c
	$(TOOLPATH)/i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu11 -c ./src/kernel/mprocessfifo.c -o ./build/kernel/mprocessfifo.o

./build/main.o: ./src/main.cpp
	clang++ -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++17 -c src/main.cpp
	ld.lld --entry KernelMain -z norelro --image-base 0x100000 --static -o kernel.elf main.o
clean:
	rm -rf ./bin/os.bin
	rm -rf ./bin/boot.bin
	rm -rf ./bin/kernel.bin
	rm -rf $(FILES)
	rm -rf ./build/*
	rm -rf ./bin/*

compile64: $(PJHOME)/UnagiLoaderPkg/Main.c
	cd $(EDK2PATH); source $(EDK2PATH)/edksetup.sh && build

run64:
	$(QEMUPATH)/qemu-system-x86_64 \
	-m 1G \
	-drive if=pflash,format=raw,readonly=on,file=$(TOOLPATH64)/OVMF_CODE.fd \
	-drive if=pflash,format=raw,file=$(TOOLPATH64)/OVMF_VARS.fd \
	-drive if=ide,index=0,media=disk,format=raw,file=$(DISK_IMG) \
	-device nec-usb-xhci,id=xhci \
	-device usb-mouse -device usb-kbd \
	-monitor stdio \
	-D /run/shm/log.txt


rungui64:
	$(QEMUPATH)/qemu-system-i386 -hda ./bin/os.bin -vga std -usbdevice tablet

gdb64:
	$(QEMUPATH)/qemu-system-x86_64 \
	-m 1G \
	-drive if=pflash,format=raw,readonly=on,file=$(TOOLPATH64)/OVMF_CODE.fd \
	-drive if=pflash,format=raw,file=$(TOOLPATH64)/OVMF_VARS.fd \
	-drive if=ide,index=0,media=disk,format=raw,file=$(DISK_IMG) \
	-device nec-usb-xhci,id=xhci \
	-device usb-mouse -device usb-kbd \
	-S -gdb tcp:127.0.0.1:1234

makeimg64:
	$(QEMUPATH)/qemu-img create -f raw $(DISK_IMG) 200M
	mkfs.fat -n 'Unagi OS' -s 2 -f 2 -R 32 -F 32 $(DISK_IMG)
	mkdir -p $(MNT_POINT)
	sudo mount -o loop $(DISK_IMG) $(MNT_POINT) && sudo mkdir -p $(MNT_POINT)/EFI/BOOT && \
	sudo cp $(EDK2PATH)/Build/UnagiLoaderX64/DEBUG_CLANGPDB/X64/Loader.efi $(MNT_POINT)/EFI/BOOT/BOOTX64.EFI
	sudo cp $(EDK2PATH)/Build/UnagiLoaderX64/DEBUG_CLANGPDB/X64/Loader.efi $(PJHOME)/build/
	sudo cp $(PJHOME)/build/kernel.elf $(MNT_POINT)/kernel.elf
	sleep 0.5
	sudo umount $(DISK_IMG)

compilekernel64:
	clang++ $(CLANG_CPPFLAGS) -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++17 -c $(PJHOME)/src/main.cpp -o $(PJHOME)/build/main.o
	ld.lld $(LD_LLDFLAGS) --entry KernelMain -z norelro --image-base 0x100000 --static -o $(PJHOME)/build/kernel.elf $(PJHOME)/build/main.o

