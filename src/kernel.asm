; kernel.asm
[BITS 32]						; All code underneath will be seen as 32 bits code
global _start						; export the symbol
extern kernel_main					; the C function
global problem
extern idt_zero

CODE_SEG equ 0x08
DATA_SEG equ 0x10					; as explained in u09, boot.asm

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

enable_a20_line:
    in al, 0x92
    test al, 2
    jnz after
    or al, 2
    and al, 0xFE
    out 0x92, al

after:
    call kernel_main
    jmp $						; infinite jump to the current position

problem:
    ; call _int0h_handler				; int 0
    int 1

times 4096-($ - $$) db 0


