; section .asm progbits  alloc   exec    nowrite  align=16		; same as .text default
section .text				; TODO so that the symbol is recognized in gdb. Properly fix by setting the linking/loading address

[BITS 32]

global _io_write_mem8:function

global _io_hlt, _io_cli, _io_sti, _io_stihlt
global _io_in8, _io_in16, _io_in32
global _io_out8, _io_out16, _io_out32
global _io_get_eflags, _io_set_eflags

_io_hlt: ; void _io_hlt(void);
    HLT
    RET

_io_cli: ; void _io_cli(void);
    CLI
    RET

_io_sti: ; void _io_sti(void);
    STI
    RET

_io_stihlt: ; void _io_stihlt(void);
    STI
    HLT
    RET

_io_in8: ; uint8_t _io_in8(uint16_t port);
    push ebp
    mov ebp, esp

    mov edx, [ebp+8]
    in al, dx				; eax is normally used to store the return value

    pop ebp
    ret

_io_in16: ; uint16_t _io_in16(uint16_t port);
    push ebp
    mov ebp, esp

    mov edx, [ebp+8]
    in ax, dx

    pop ebp
    ret

_io_in32: ; uint32_t _io_in32(uint16_t port);
    push ebp
    mov ebp, esp

    mov edx, [ebp+8]
    in eax, dx

    pop ebp
    ret

_io_out8: ; uint8_t _io_out8(uint16_t port, uint8_t val);
    push ebp
    mov ebp, esp

    mov eax, [ebp+12]			; val
    mov edx, [ebp+8]
    out dx, al

    pop ebp
    ret

_io_out16: ; uint16_t _io_out16(uint16_t port, uint16_t val);
    push ebp
    mov ebp, esp

    mov eax, [ebp+12]
    mov edx, [ebp+8]
    out dx, ax

    pop ebp
    ret

_io_out32: ; uint32_t _io_out32(uint16_t port, uint32_t val);
    push ebp
    mov ebp, esp

    mov eax, [ebp+12]
    mov edx, [ebp+8]
    out dx, eax

    pop ebp
    ret

_io_get_eflags: ; uint32_t _io_get_eflags(void);
    pushfd
    pop eax
    ret

_io_set_eflags: ; uint32_t _io_set_eflags(uint32_t eflags);
    push ebp
    mov ebp, esp

    mov eax, [ebp+8]
    push eax
    popfd

    pop ebp
    ret

_io_write_mem8: ; void write_mem8(uint32_t addr, uint8_t data);
    push ebp
    mov ebp, esp

    mov ecx, [esp + 8]			; addr
    mov al, [esp + 12]			; data
    mov [ecx], al

    pop ebp
    ret



times 512-($ - $$) db 0
