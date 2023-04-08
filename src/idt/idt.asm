; section .asm
section .text

global idt_load				; globally export the symbol
global int21h
extern _int21h_handler
global interrupt_pointer_table
extern _interrupt_handler

global enable_interrupts
global disable_interrupts

enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret

idt_load:
    push ebp
    mov ebp, esp

    xor ebx, ebx
    mov ebx, [ebp+8]			; Pass the first arg to ebx
    lidt [ebx]				; Load Interrupt Descriptor Table (IDT)

    pop ebp
    ret

int21h:					; ISA IRT 2, Keyboard Input
    cli
    pushad

    xor eax, eax
    in al, 60h				; Read key
    push ax
    call _int21h_handler
    pop ax				; Reverse esp

    popad
    sti
    iretd				; IRET and IRETD are mnemonics for the same opcode. The IRETD mnemonic (interrupt return double) is intended for use when returning from an interrupt when using the 32-bit operand size; however, most assemblers use the IRET mnemonic interchangeably for both operand sizes

%macro interrupt 1
    global int%1
    int%1:
        ; INTERRUPT FRAME START
	; PUSHED TO US BY THE PROCESSOR UPON ENTRY TO THIS INTERRUPT
	; uint32_t ip
	; uint32_t cs
	; uint32_t flags
	; uint32_t sp
	; uint32_t ss
	pushad

	push esp			; the "stack frame"
	push dword %1
	call _interrupt_handler
	add esp, 8			; to reverse the two push

	popad
	iretd
%endmacro

%assign i 0
%rep 256
    interrupt i
%assign i i+1
%endrep

times 4096-($ - $$) db 0

section .data

%macro interrupt_array_entry 1		; define a macro named interrupt_array_entry with one variable
    dd int%1				; "dd int$1"; which will be further replaced by the address of corresponding tags
%endmacro

interrupt_pointer_table:
%assign i 0
%rep 256
    interrupt_array_entry i
%assign i i+1
%endrep

times 4096-($ - $$) db 0
