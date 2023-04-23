; kernel.asm
[BITS 32]
global _start						; export the symbol
extern kernel_main					; the C function

_start:
    call kernel_main
    jmp $						; infinite jump to the current position

times 4096-($ - $$) db 0


