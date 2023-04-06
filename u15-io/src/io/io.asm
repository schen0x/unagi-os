section .asm

global insb
global insw
global outb
global outw

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

times 4096-($ - $$) db 0
