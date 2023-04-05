section .asm

global idt_load				; globally export the symbol
idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8]			; Pass the first arg to ebx
    lidt [ebx]				; Load Interrupt Descriptor Table (IDT)

    pop ebp
    ret

times 4096-($ - $$) db 0
