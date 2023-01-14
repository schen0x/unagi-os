all:
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin
	# dd if=./message.txt >> ./latest.bin
	# dd if=/dev/zero bs=512 count=1 >> ./boot.bin
	# qemu-system-x86_64 -hda ./latest.bin -curses -S -s
	# qemu-system-x86_64 -hda ./bin/boot.bin -curses -S -gdb tcp:127.0.0.1:1234

clean:
	rm -rf ./bin/boot.bin

