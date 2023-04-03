; Improve the Bootloader by change origin and using Segment Register
; Because the DS and other segment registers are unknown, the print is not guaranteed to success on most systems
[BITS 16]						; Tells nasm this is 16 bit code, because BIOS runs in 16 bits
[ORG 0]

jmp 0x7c0:START						; Because our code will still be loded to 0x7c0

START:
    cli				; Clear Interrupts, because the following ops must not be interrupted
    mov ax, 0x7c0
    mov ds, ax			; Initialize the Segment Registers, this increases control
    mov es, ax			; Extra Segment Register, another data segment
    mov ax, 0x00
    mov ss, ax
    mov sp, 0x7c00		; The stack grows downwards in x86 systems. So [0-0x7c00] is the stack, to prevent the stack from overwriting the bootloader itself
    sti				; Enables Interrupts
    mov si, MESSAGE
    call PRINT
    jmp $

MESSAGE:
    db 'Hello World!', 0

PRINT:
    mov bx, 0
.LOOP:				; sub-label
    lodsb			; load 1 char from [DS:SI] to al then increment si
    cmp al, 0
    je .DONE
    call PRINT_CHAR
    jmp .LOOP
.DONE:
    ret

PRINT_CHAR:			; Subroutine
    mov ah, 0eh			; al should be already pointing to the char
    int 0x10
    ret

times 510 - ($ - $$) db 0	; pad to write the boot signature 0x55 0xAA
dw 0xAA55			; The boot signature in Little Endian

