; ENTER PROTECTED MODE
[BITS 16]
[ORG 0x7c00]

CODE_SEG equ GDT_CODE - GDT_START
DATA_SEG equ GDT_DATA - GDT_START

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
    mov sp, 0x7c00
    sti

.LOAD_PROTECTED
    cli
    lgdt[GDT_DESCRIPTOR]				; load GDT with GDTR
    mov eax, cr0
    or al, 1						; set PE (Protection Enable) bit in CR0 (Control Register 0)
    mov cr0, eax

    ; to load CS with proper PM32 descriptor
    jmp CODE_SEG:LOAD32					 ; Perform far jump to selector 08h (offset into GDT, pointing at a 32bit PM code segment descriptor)

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
    db 11001111b					; Simply allow all memory access
    db 0						; Base 31:24

GDT_DATA:						; DS, SS, ES, FS, GS
    dw 0xffff						; Segment Limit, first 0-15 bits, 0xffff is 256MB in 4kb page
    dw 0						; Base Address 15:00
    db 0						; Base Address 23:16
    db 0x92						; Access Byte for Kernel Mode Data Segment
    db 11001111b
    db 0						; Base 31:24

GDT_END:

GDT_DESCRIPTOR:						; GDTR
    dw GDT_END - GDT_START - 1				; GDT_Size - 1, because max GDT_Size can be 65536, 1 byte more than 0xffff
    dd GDT_START					; Offset

[BITS 32]
LOAD32:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp
    jmp $

times 510 - ($ - $$) db 0	; pad to write the boot signature 0x55 0xAA
dw 0xAA55			; The boot signature in Little Endian
