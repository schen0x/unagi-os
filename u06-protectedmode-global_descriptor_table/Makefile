all:
	nasm -f bin ./*-latest.asm -o ./latest.bin
	dd if=./message.txt >> ./latest.bin
	dd if=/dev/zero bs=512 count=1 >> ./latest.bin
	# qemu-system-x86_64 -hda ./latest.bin -curses -S -s
	qemu-system-x86_64 -hda ./latest.bin -curses -S -gdb tcp:127.0.0.1:1234
