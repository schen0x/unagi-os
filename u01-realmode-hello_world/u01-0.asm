[BITS 16]
[ORG 0x7c00]

jmp short START
nop

START:
    mov	ah, 0eh			; print 1 char
    mov	al, 'A'
    mov bx, 15			; colorcode
    int 0x10

times 510 - ($ - $$) db 0
dw 0xAA55
