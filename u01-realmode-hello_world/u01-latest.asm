; The minimum image
; BIOS will search for boot signature 0x55 0xAA, on all devices
; When Found, it load the sector into 0x7c00 and execute
[BITS 16]						; Tells nasm this is 16 bit code, because BIOS runs in 16 bits
[ORG 0x7c00]						; Origin; tells nasm the code will be loaded to memory 0x0, because BIOS will load the bootable section to 0x7c00

; jmp short START					; convension, but not a must
; nop

START:
    mov si, MESSAGE
    call PRINT
    jmp $

MESSAGE:
    db 'Hello World!', 0

PRINT:
    mov bx, 0
.LOOP:				; sub-label
    lodsb			; load 1 char from [si] to al then increment si
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
