# FILES = ./build/kernel.asm.o ./build/kernel.o ./build/idt/idt.asm.o ./build/idt/idt.o ./build/memory/memory.o ./build/util/kutil.o ./build/gdt/gdt.asm.o ./build/gdt/gdt.o
FILES = ./build/kernel.asm.o ./build/kernel.o ./build/idt/idt.asm.o ./build/idt/idt.o ./build/memory/memory.o ./build/util/kutil.o ./build/io/io.asm.o ./build/io/io.o ./build/pic/pic.asm.o ./build/pic/pic.o ./build/drivers/keyboard.o ./build/memory/heap.o ./build/memory/kheap.o ./build/memory/paging/paging.o ./build/memory/paging/paging.asm.o
GCC_KERNEL_INCLUDES = -I./src
# GCC_KERNEL_FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc
GCC_KERNEL_FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -O0 -Iinc

all: clean builddir compile run
allgdb: clean builddir compile gdb
allgui: clean builddir compile rungui

builddir:
	mkdir -p build/gdt build/idt build/memory build/memory/paging build/util build/io build/pic build/drivers

compile: ./bin/boot.bin ./bin/kernel.bin
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=512 count=100 >> ./bin/os.bin

run:
	qemu-system-i386 -hda ./bin/os.bin -curses

rungui:
	qemu-system-i386 -hda ./bin/os.bin

gdb:
	# qemu-system-x86_64 -hda ./bin/os.bin -curses -S -s
	qemu-system-i386 -hda ./bin/os.bin -curses -S -gdb tcp:127.0.0.1:1234

# The bootloader
./bin/boot.bin: ./src/boot/boot.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin
# The 32 bits kernel, depends on two parts, written in asm and C
./bin/kernel.bin: $(FILES)
	i686-elf-ld -g -relocatable $(FILES) -o ./build/kernelfull.o
	# i686-elf-gcc -T ./src/linker.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/kernelfull.o
	i686-elf-ld -T ./src/linker.ld -o ./bin/kernel.bin -O0 ./build/kernelfull.o
./build/kernel.asm.o: ./src/kernel.asm
	nasm -f elf -g ./src/kernel.asm -o ./build/kernel.asm.o
./build/kernel.o: ./src/kernel.c
	i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu99 -c ./src/kernel.c -o ./build/kernel.o
./build/idt/idt.asm.o: ./src/idt/idt.asm
	nasm -f elf -g ./src/idt/idt.asm -o ./build/idt/idt.asm.o
./build/idt/idt.o: ./src/idt/idt.c
	i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu99 -c ./src/idt/idt.c -o ./build/idt/idt.o
./build/memory/memory.o: ./src/memory/memory.c
	i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu99 -c ./src/memory/memory.c -o ./build/memory/memory.o
./build/util/kutil.o: ./src/util/kutil.c
	i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu99 -c ./src/util/kutil.c -o ./build/util/kutil.o
# ./build/gdt/gdt.asm.o: ./src/gdt/gdt.asm
	# nasm -f elf -g ./src/gdt/gdt.asm -o ./build/gdt/gdt.asm.o
# ./build/gdt/gdt.o: ./src/gdt/gdt.c
	# i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu99 -c ./src/gdt/gdt.c -o ./build/gdt/gdt.o
./build/io/io.asm.o: ./src/io/io.asm
	# nasm -f elf -g ./src/io/io.asm -o ./build/io/io.asm.elf
	#i686-elf-ld -m elf_i386 -o ./build/io/io.asm.o ./build/io/io.asm.elf
	nasm -f elf -F dwarf -g ./src/io/io.asm -o ./build/io/io.asm.o
./build/io/io.o: ./src/io/io.c
	i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu99 -c ./src/io/io.c -o ./build/io/io.o
./build/pic/pic.asm.o: ./src/pic/pic.asm
	nasm -f elf -F dwarf -g ./src/pic/pic.asm -o ./build/pic/pic.asm.o
./build/pic/pic.o: ./src/pic/pic.c
	i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu99 -c ./src/pic/pic.c -o ./build/pic/pic.o
./build/drivers/keyboard.o: ./src/drivers/keyboard.c
	i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu99 -c ./src/drivers/keyboard.c -o ./build/drivers/keyboard.o
./build/memory/heap.o: ./src/memory/heap.c
	i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu99 -c ./src/memory/heap.c -o ./build/memory/heap.o
./build/memory/kheap.o: ./src/memory/kheap.c
	i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu99 -c ./src/memory/kheap.c -o ./build/memory/kheap.o
./build/memory/paging/paging.o: ./src/memory/paging/paging.c
	i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu99 -c ./src/memory/paging/paging.c -o ./build/memory/paging/paging.o
./build/memory/paging/paging.asm.o: ./src/memory/paging/paging.asm
	nasm -f elf -F dwarf -g ./src/memory/paging/paging.asm -o ./build/memory/paging/paging.asm.o

clean:
	rm -rf ./bin/os.bin
	rm -rf ./bin/boot.bin
	rm -rf ./bin/kernel.bin
	rm -rf $(FILES)
	rm -rf ./build/*
	rm -rf ./bin/*
