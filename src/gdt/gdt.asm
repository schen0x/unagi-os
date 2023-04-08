section .asm

global gdt_load				; globally export the symbol
gdt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8]			; Pass the first arg to ebx
    lgdt [ebx]				; Load Global Descriptor Table (GDT)

    pop ebp
    ret

times 4096-($ - $$) db 0
