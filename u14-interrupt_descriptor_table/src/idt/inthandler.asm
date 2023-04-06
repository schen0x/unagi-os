section .asm
global _entry_inthandler_1
extern _inthandler_1

_entry_inthandler_1:
    push ebp
    mov ebp, esp

    ; mov ebx, [ebp+8]			; Pass the first arg to ebx
    call _inthandler_1
    jmp $				; ? FIXME pause exec

    pop ebp
    iretd				; IRET and IRETD are mnemonics for the same opcode. The IRETD mnemonic (interrupt return double) is intended for use when returning from an interrupt when using the 32-bit operand size; however, most assemblers use the IRET mnemonic interchangeably for both operand sizes

times 4096-($ - $$) db 0
