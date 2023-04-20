; section .asm progbits  alloc   exec    nowrite  align=16		; same as .text default
section .text				; TODO so that the symbol is recognized in gdb. Properly fix by setting the linking/loading address

global insb:function
global insw:function
global outb:function
global outw:function
global _write_mem8:function

insb:
    push ebp
    mov ebp, esp

    xor eax,eax
    mov edx, [ebp+8]
    in al,dx				; eax is normally used to store the return value

    pop ebp
    ret

insw:
    push ebp
    mov ebp, esp


    xor eax,eax
    mov edx, [ebp+8]
    in ax,dx				; eax is normally used to store the return value

    pop ebp
    ret

outb:
    push ebp
    mov ebp, esp


    xor eax,eax
    mov eax, [ebp+12]			; the byte to write
    mov edx, [ebp+8]
    out dx,al

    pop ebp
    ret

outw:
    push ebp
    mov ebp, esp


    xor eax,eax
    mov eax, [ebp+12]			; the word to write
    mov edx, [ebp+8]
    in ax,dx

    pop ebp
    ret

_write_mem8: ; void write_mem8(uint32_t addr, uint8_t data);
    push ebp
    mov ebp, esp

    mov ecx, [esp + 8]			; addr
    mov al, [esp + 12]			; data
    mov [ecx], al

    pop ebp
    ret

times 512-($ - $$) db 0
