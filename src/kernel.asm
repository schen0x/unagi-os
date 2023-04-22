; kernel.asm
[BITS 32]						; All code underneath will be seen as 32 bits code
global _start						; export the symbol
extern kernel_main					; the C function
global problem
extern idt_zero

CODE_SEG equ 0x10
DATA_SEG equ 0x8					; as explained in u09, boot.asm

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

after:
    call kernel_main
    jmp $						; infinite jump to the current position

problem:
    ; call _int0h_handler				; int 0
    int 1

times 4096-($ - $$) db 0


