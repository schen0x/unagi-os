; Customize interrupt handler, modify the Interrupt Vector Table (IVT)
[BITS 16]
[ORG 0]
_START:
    jmp short START
    nop

times 33 db 0						; Fake BPB (BIOS Parameter Block), 36 bytes (BPB) - 3 bytes (JMP SHORT START NOP) = 33 bytes

START:
    jmp 0x7c0:STEP2

HANDLE_ZERO:
    mov ah, 0eh
    mov al, 'A'
    mov bx, 0x00
    int 0x10
    iret

STEP2:
    cli
    mov ax, 0x7c0
    mov ds, ax
    mov es, ax
    mov ax, 0x00
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov word[ss:0x00], HANDLE_ZERO			; 2 bytes; The offset, if not ss not specified, will be default to use ds
    mov word[ss:0x02], 0x7c0				; 2 bytes; if not ss not specified, the default to use ds
    int 0						; call our int0 manually
    mov ax, 0x00
    div ax						; int0 also happens to be divide by 0 exception, we can trigger it by div 0
    mov si, MESSAGE
    call PRINT
    jmp $

MESSAGE:
    db 'Hello World!', 0

PRINT:
    mov bx, 0
.LOOP:
    lodsb
    cmp al, 0
    je .DONE
    call PRINT_CHAR
    jmp .LOOP
.DONE:
    ret

PRINT_CHAR:
    mov ah, 0eh			; al should be already pointing to the char
    int 0x10
    ret

times 510 - ($ - $$) db 0	; pad to write the boot signature 0x55 0xAA
dw 0xAA55			; The boot signature in Little Endian

