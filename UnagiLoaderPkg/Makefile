PJHOME = ..
QEMUPATH = /usr/local/bin
EDK2PATH = $(HOME)/src/edk2
TOOLPATH = $(PJHOME)/modules/unagios-build/devenv
SHELL := /bin/bash
DISK_IMG = $(PJHOME)/build/disk.img
MNT_POINT = $(PJHOME)/mnt

all: clean builddir compile makeimg run
allcompile: clean builddir compile
allgdb: clean builddir compile makeimg gdb

compile: ./Main.c
	cd $(EDK2PATH); source $(EDK2PATH)/edksetup.sh && build

builddir:
	mkdir -p $(PJHOME)/build/

run:
	$(QEMUPATH)/qemu-system-x86_64 \
    	-m 1G \
    	-drive if=pflash,format=raw,readonly=on,file=$(TOOLPATH)/OVMF_CODE.fd \
    	-drive if=pflash,format=raw,file=$(TOOLPATH)/OVMF_VARS.fd \
    	-drive if=ide,index=0,media=disk,format=raw,file=$(DISK_IMG) \
    	-device nec-usb-xhci,id=xhci \
    	-device usb-mouse -device usb-kbd \

	# $(QEMUPATH)/qemu-system-i386 -hda ./bin/os.bin -vga std -curses

rungui:
	$(QEMUPATH)/qemu-system-i386 -hda ./bin/os.bin -vga std -usbdevice tablet

gdb:
	$(QEMUPATH)/qemu-system-x86_64 \
    	-m 1G \
    	-drive if=pflash,format=raw,readonly=on,file=$(TOOLPATH)/OVMF_CODE.fd \
    	-drive if=pflash,format=raw,file=$(TOOLPATH)/OVMF_VARS.fd \
    	-drive if=ide,index=0,media=disk,format=raw,file=$(DISK_IMG) \
    	-device nec-usb-xhci,id=xhci \
    	-device usb-mouse -device usb-kbd \
		-S -gdb tcp:127.0.0.1:1234

makeimg:
	$(QEMUPATH)/qemu-img create -f raw $(DISK_IMG) 200M
	mkfs.fat -n 'Unagi OS' -s 2 -f 2 -R 32 -F 32 $(DISK_IMG)
	mkdir -p $(MNT_POINT)
	sudo mount -o loop $(DISK_IMG) $(MNT_POINT) && sudo mkdir -p $(MNT_POINT)/EFI/BOOT && \
	sudo cp $(EDK2PATH)/Build/UnagiLoaderX64/DEBUG_CLANGPDB/X64/Loader.efi $(MNT_POINT)/EFI/BOOT/BOOTX64.EFI
	sudo cp $(EDK2PATH)/Build/UnagiLoaderX64/DEBUG_CLANGPDB/X64/Loader.efi $(PJHOME)/build/
	sleep 0.5
	sudo umount $(DISK_IMG)

clean:
	rm -rf $(PJHOME)/build/*
	rm -rf $(PJHOME)/bin/*
	rm -rf $(DISK_IMG)
