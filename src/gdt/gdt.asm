section .text

global _gdt_reload:FUNCTION		; globally export the symbol
_gdt_reload:				; Reload the gdt (can be used in protected mode); Especially careful about the DS CS SS values, because if they did not point to the correct segment, the original code flow cannot resume
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8]			; Pass the first arg to ebx
    lgdt [ebx]				; lgdt m16&32; Load Global Descriptor Table (GDT)

    pop ebp
    ret

global _gdt_ltr:FUNCTION
_gdt_ltr:
    push ebp
    mov ebp, esp

    ; mov WORD ax, [ebp + 8]
    ; ltr ax				; ALSO OK
    ltr [ebp+8]				; ltr r/m16; Load Task Register; The source operand (a general-purpose register or a memory location) contains a segment selector that points to a task state segment (TSS). After the segment selector is loaded in the task register, the processor uses the segment selector to locate the segment descriptor for the TSS in the global descriptor table (GDT). It then loads the segment limit and base address for the TSS from the segment descriptor into the task register. The task pointed to by the task register is marked busy, but a switch to the task does not occur.

    pop ebp
    ret

global _taskswitch4:FUNCTION
; TODO (Modern Implementation and proper implementation?)
_taskswitch4: ; void _taskswitch4(void);
    jmp 4*8:0
    RET

global _taskswitch3:FUNCTION
_taskswitch3: ; void _taskswitch3(void);
    jmp 3*8:0
    RET



times 4096-($ - $$) db 0
