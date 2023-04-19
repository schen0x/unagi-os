[BITS 16]
[ORG 0x7c00]						; Memory Map - Boot Sector is loaded at this address

PROTECTED_MODE_CODE_SEGMENT_SELECTOR equ  1000b		; Index 1, no flags set; The first entry in the GDTR
PROTECTED_MODE_DATA_SEGMENT_SELECTOR equ 10000b		; Index 2, no flags set; The second entry in the GDTR
CODE_SEG equ GDT_CODE - GDT_START			; This also calculates the offset. Because each Segment Descriptor is 64 bits or 8 bytes. Offset * 8 equals to the segment selector with no flag.
DATA_SEG equ GDT_DATA - GDT_START
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
    mov cl, 2						; Sector

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
    cmp cl, 63						; disk CHS max sector convention
    jbe .int13h_readloop				; loop when below equal x sectors
    sti
    ; jmp NEXT_SECTORS
    jmp 0:LOAD_PROTECTED

;VESA_INIT:
;    pusha
;    pushf
;    xor ah,ah
;    mov al, 0x13					; 300x200x256
;    ;mov al, 0x107					; 107h   1280x1024x256 VESA
;    int 0x10
;
;    popf
;    popa


LOAD_PROTECTED:
    cli
    mov ax, 0x00
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00					; The stack grows downwards in x86 systems. So [0-0x7c00] is the stack, to prevent the stack from overwriting the bootloader itself.
    lgdt[GDT_DESCRIPTOR]				; load GDT with GDT_DESCRIPTOR (GDTR)
    mov eax, cr0					; Prepare to set PE (Protection Enable) bit in CR0 (Control Register 0)
    or al, 1						; Prepare to set PE bit in CR0
    mov cr0, eax					; Set PE (Protection Enable) bit in CR0 (Control Register 0)

    jmp PROTECTED_MODE_CODE_SEGMENT_SELECTOR:load32	; load the 32 bits code into memory and jump to it

; GDT
GDT_START:
GDT_NULL:						; GDT Entry 0
    dd 0						; 4 bytes
    dd 0

; OFFSET 0x0
GDT_CODE:						; CS should point to this
    dw 0xffff						; Segment Limit, first 0-15 bits, 0xffff is 256MB in 4kb page
    dw 0						; Base Address 15:00
    db 0						; Base Address 23:16
    db 10011010b					; Common Access Byte 0x9a for Kernel Mode Code Segment
    db 11001111b					; Flags and Limit; Simply allow all memory access
    db 0						; Base 31:24

GDT_DATA:						; DS, SS, ES, FS, GS
    dw 0xffff						; Segment Limit, first 0-15 bits, 0xffff is 256MB in 4kb page
    dw 0						; Base Address 15:00
    db 0						; Base Address 23:16
    db 0x92						; Access Byte for Kernel Mode Data Segment
    db 11001111b					; Flags and Limit
    db 0						; Base 31:24

GDT_END:

GDT_DESCRIPTOR:						; The GDTR
    dw GDT_END - GDT_START - 1				; GDT_Size - 1, because max GDT_Size can be 65536, 1 byte more than 0xffff
    dd GDT_START					; Offset, 4 bytes in 32 bit mode


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 32 BIT START ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
[BITS 32]
load32:
    mov eax, 1						; we use eax to represent the starting sector to load
    mov ecx, 100					; we use ecx to represent the total sectors to load
    mov edi, 0x0100000					; we use edi to represent the dst address when loading
    call ata_lba_read
    jmp CODE_SEG:0x0100000				; jump to 1MB

ata_lba_read:
    mov ebx, eax					; Backup the Logic Block Address (LBA)
    ; Send the highest 8 bits of the lba to hard disk controller
    shr eax, 24
    or eax, 0xE0					; select the master drive
    mov dx, 0x1F6					; the destined port
    out dx, al
    ; Finished sending the highest 8 bits of the lba

    ; Send the total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ; Finished sending the total bits to read

    ; Send more bits of the LBA
    mov eax, ebx					; Restore the backup LBA
    mov dx, 0x1F3
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send more bits of the LBA
    mov dx, 0x1F4
    mov eax, ebx					; Restore the backup LBA
    shr eax, 8
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send the upper 16 bits of the LBA
    mov dx, 0x1F5
    mov eax, ebx					; Restore the backup LBA
    shr eax, 16
    out dx, al
    ; Finished sending the upper 16 bits of the LBA

    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

; Read all sectors into memory
.next_sector:
    push ecx

; Checking if we need to read
.try_again:
    mov dx, 0x1f7
    in al, dx
    test al, 8
    jz .try_again

; We need to read 256 words at a time
    mov ecx, 256
    mov dx, 0x1F0
    rep insw						; Read one word, repeat 256 times
    pop ecx						; Restore the ecx
    loop .next_sector					; Loop and decrement ecx by 1
    ; End of reading sectors to read
    ret

times 510 - ($ - $$) db 0	; pad to write the boot signature 0x55 0xAA
dw 0xAA55			; The boot signature in Little Endian
