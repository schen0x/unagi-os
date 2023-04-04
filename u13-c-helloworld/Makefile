FILES = ./build/kernel.asm.o ./build/kernel.o
GCC_KERNEL_INCLUDES = -I./src
# GCC_KERNEL_FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc
GCC_KERNEL_FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -O0 -Iinc

all: ./bin/boot.bin ./bin/kernel.bin
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=512 count=100 >> ./bin/os.bin
	# qemu-system-x86_64 -hda ./bin/os.bin -curses -S -s
	# qemu-system-x86_64 -hda ./bin/os.bin -curses -S -gdb tcp:127.0.0.1:1234
	# qemu-system-x86_64 -hda ./bin/os.bin -curses -S
	qemu-system-x86_64 -hda ./bin/os.bin -curses

# The bootloader
./bin/boot.bin: ./src/boot/boot.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin

# The 32 bits kernel, depends on two parts, written in asm and C
./bin/kernel.bin: $(FILES)
	i686-elf-ld -g -relocatable $(FILES) -o ./build/kernelfull.o
	i686-elf-gcc -T ./src/linker.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/kernelfull.o

./build/kernel.asm.o: ./src/kernel.asm
	nasm -f elf -g ./src/kernel.asm -o ./build/kernel.asm.o

./build/kernel.o: ./src/kernel.c
	i686-elf-gcc $(GCC_KERNEL_INCLUDES) $(GCC_KERNEL_FLAGS) -std=gnu99 -c ./src/kernel.c -o ./build/kernel.o

clean:
	rm -rf ./bin/os.bin
	rm -rf ./bin/boot.bin
	rm -rf ./bin/kernel.bin
	rm -rf $(FILES)
