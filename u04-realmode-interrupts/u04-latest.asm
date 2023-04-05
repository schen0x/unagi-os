; Customize interrupt handler, modify the Interrupt Vector Table (IVT)
[BITS 16]
[ORG 0]
_START:
    jmp short START
    nop

times 33 db 0						; Fake BPB (BIOS Parameter Block), 36 bytes (BPB) - 3 bytes (JMP SHORT START NOP) = 33 bytes

START:
    jmp 0x7c0:STEP2

HANDLE_ZERO:						; The interrupt 0 subroutine
    mov ah, 0eh
    mov al, 'A'
    mov bx, 0x00
    int 0x10						; Print the char 'A' onto
    iret

STEP2:
    cli
    mov ax, 0x7c0
    mov ds, ax
    mov es, ax
    mov ax, 0x00
    mov ss, ax
    mov sp, 0x7c00
    sti								; Enable Interrupts

	; directly modify the IVT table entry 0, which is the 4 bytes start from 0x0
    mov word[ss:0x00], HANDLE_ZERO	; The IVT Offset; mov 2 bytes; `ss` is necessary because otherwise `ds` is used by default during a `mov` instruction
    mov word[ss:0x02], 0x7c0		; The IVT Segment
    int 0							; Invoke the Interrupt 0
    mov ax, 0x00
    div ax							; int0 also happens to be divide by 0 exception, we can trigger it by div 0
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

