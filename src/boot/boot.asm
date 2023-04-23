[BITS 16]
[ORG 0x7c00]						; Memory Map - Boot Sector is loaded at this address

BOOT_DISK_INT13H_DRIVE_TYPE equ 80h			; 1st hard disk
LOAD_ADDRESS_NEXT_SECTOR_ES equ 820h			; ES * 0x10h == 0x8200

_START:
    jmp short START
    nop

times 33 db 0						; Fake BPB (BIOS Parameter Block), 36 bytes (BPB) - 3 bytes (JMP SHORT START NOP) = 33 bytes

START:
    jmp 0:STEP2

STEP2:
    cli
    mov ax, 0x00
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00					; The stack grows downwards in x86 systems. So [0-0x7c00] is the stack, to prevent the stack from overwriting the bootloader itself.
    sti
    jmp 0:READ_MORE_SECTORS
    ;jmp 0:LOAD_PROTECTED

READ_MORE_SECTORS:					; load more sectors to [0x7e00 - 0x7ffff] (conventional memory), for disk, use ah=8 function to get CHS info but we only load very few sectors for video etc. See LBA to CHS algorithm in OSDEV
    cli
    mov ax, LOAD_ADDRESS_NEXT_SECTOR_ES
    mov es, ax						; ES:BX Buffer Address Pointer, ES * 0x10 == 0x8200
    mov ch, 0						; Cylinder
    mov dh, 0						; Head
    mov cl, 2						; CHS Sector, 1 indexed

.int13h_readloop:
    mov si, 0						; Fail count
.int13h_retry:
    mov ah, 0x02					; INT 13h AH=02h Read disk
    mov al, 1						; Sectors To Read Count
    mov bx, 0						; ES:BX Buffer Address Pointer
    mov dl, BOOT_DISK_INT13H_DRIVE_TYPE
    int 0x13						; CF is set on error, clear if no error
    jnc .int13h_next
    add si, 1						; retry counter++
    cmp si, 5						; retry max = 5
    mov ah, 0
    mov dl, BOOT_DISK_INT13H_DRIVE_TYPE
    int 0x13						; drive reset
    jmp .int13h_retry

.int13h_next:
    mov ax, es
    add ax, 0x0020
    mov es, ax						; 0x200 == 512 byte == 1 sector
    add cl, 1
    cmp cl, 4						; VOLATILE; disk CHS max sector 63
    jbe .int13h_readloop				; loop when below equal x sectors. load 4096 bytes, which is boot.asm + boot_next.asm
    sti
    jmp LOAD_ADDRESS_NEXT_SECTOR_ES * 0x10
    ; jmp 0:LOAD_PROTECTED

times 510 - ($ - $$) db 0	; pad to write the boot signature 0x55 0xAA
dw 0xAA55			; The boot signature in Little Endian

