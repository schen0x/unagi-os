[BITS 16]
[ORG 0x7c00]

PROTECTED_MODE_CODE_SEGMENT_SELECTOR equ  1000b		; Index 1, no flags set; The first entry in the GDTR
PROTECTED_MODE_DATA_SEGMENT_SELECTOR equ 10000b		; Index 2, no flags set; The second entry in the GDTR
CODE_SEG equ GDT_CODE - GDT_START			; This also calculates the offset. Because each Segment Descriptor is 64 bits or 8 bytes. Offset * 8 equals to the segment selector with no flag.
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
    mov sp, 0x7c00					; The stack grows downwards in x86 systems. So [0-0x7c00] is the stack, to prevent the stack from overwriting the bootloader itself.
    sti

.LOAD_PROTECTED:
    cli
    lgdt[GDT_DESCRIPTOR]				; load GDT with GDT_DESCRIPTOR (GDTR)
    mov eax, cr0					; Prepare to set PE (Protection Enable) bit in CR0 (Control Register 0)
    or al, 1						; Prepare to set PE bit in CR0
    mov cr0, eax					; Set PE (Protection Enable) bit in CR0 (Control Register 0)

    jmp PROTECTED_MODE_CODE_SEGMENT_SELECTOR:PModeMain	; jmp 08h:Offset, The first real Mode instruction; Same as jmp CODE_SEG:PModeMain; See the Note U09 for detail

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

[BITS 32]						; All code underneath will be seen as 32 bits code
PModeMain:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

enable_a20_line:
    in al, 0x92
    test al, 2
    jnz after
    or al, 2
    and al, 0xFE
    out 0x92, al
after:
    jmp $						; infinite jump to the current position

times 510 - ($ - $$) db 0	; pad to write the boot signature 0x55 0xAA
dw 0xAA55			; The boot signature in Little Endian
