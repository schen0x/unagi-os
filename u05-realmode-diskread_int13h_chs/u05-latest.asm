; READ DATA FROM DISK (CHS, INT13)
[BITS 16]
[ORG 0]
_START:
    jmp short START
    nop

times 33 db 0						; Fake BPB (BIOS Parameter Block), 36 bytes (BPB) - 3 bytes (JMP SHORT START NOP) = 33 bytes

START:
    jmp 0x7c0:STEP2

STEP2:
    cli
    mov ax, 0x7c0
    mov ds, ax
    mov es, ax
    mov ax, 0x00
    mov ss, ax
    mov sp, 0x7c00
    sti

    ; USE INT13 TO READ DISK SECTOR
    mov ah, 2
    mov al, 1						; Number of sectors to read (nonzero)
    mov ch, 0						; Low eight bits of cylinder number
    mov cl, 2						; Read sector two
    mov dh, 0						; Head number
    							; DL has been set automatically
    mov bx, BUFFER					; Load the disk sector to BUFFER
    int 0x13
    jc ERROR						; If the carry flag is set

    mov si, BUFFER
    call PRINT
    jmp $

ERROR:
    mov si, ERROR_MESSAGE
    call PRINT
    jmp $


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

ERROR_MESSAGE:
    db "Fail to load sector"

times 510 - ($ - $$) db 0	; pad to write the boot signature 0x55 0xAA
dw 0xAA55			; The boot signature in Little Endian

BUFFER:							; Since BIOS only load the 1st sector, this will not be loaded automatically; But we can reference it
