section .text

global _gdt_reload:FUNCTION		; globally export the symbol
_gdt_reload:				; Reload the gdt (can be used in protected mode); Especially careful about the DS CS SS values, because if they did not point to the correct segment, the original code flow cannot resume
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8]			; Pass the first arg to ebx
    lgdt [ebx]				; Load Global Descriptor Table (GDT)

    pop ebp
    ret

global _gdt_load_task_register:FUNCTION
_gdt_load_task_register:
    push ebp
    mov ebp, esp

    mov WORD ebx, [ebp+8]		; ltr r/m16
    ltr [ebx]				; Load Task Register; The source operand (a general-purpose register or a memory location) contains a segment selector that points to a task state segment (TSS). After the segment selector is loaded in the task register, the processor uses the segment selector to locate the segment descriptor for the TSS in the global descriptor table (GDT). It then loads the segment limit and base address for the TSS from the segment descriptor into the task register. The task pointed to by the task register is marked busy, but a switch to the task does not occur.

    pop ebp
    ret


times 4096-($ - $$) db 0
