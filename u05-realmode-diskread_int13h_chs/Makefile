all:
	nasm -f bin ./*-latest.asm -o ./latest.bin
	dd if=./message.txt >> ./latest.bin
	dd if=/dev/zero bs=512 count=1 >> ./latest.bin
	qemu-system-x86_64 -hda ./latest.bin -curses
