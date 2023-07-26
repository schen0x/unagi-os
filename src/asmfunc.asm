; asmfunc.asm
;
; System V AMD64 Calling Convention
; Registers: RDI, RSI, RDX, RCX, R8, R9

bits 64
section .text

global IoOut32  ; void IoOut32(uint16_t addr, uint32_t data);
IoOut32:
    mov dx, di    ; dx = addr
    mov eax, esi  ; eax = data
    out dx, eax
    ret

global IoIn32  ; uint32_t IoIn32(uint16_t addr);
IoIn32:
    mov dx, di    ; dx = addr
    in eax, dx
    ret

global GetCS  ; uint16_t GetCS(void);
GetCS:
    xor eax, eax  ; also clears upper 32 bits of rax
    mov ax, cs
    ret

global LoadIDT  ; void __attribute__((sysv_abi)) LoadIDT(uint16_t limit, uint64_t offset);
LoadIDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10
    mov [rsp], di  ; limit, 1st arg
    mov [rsp + 2], rsi  ; offset, 2nd arg
    lidt [rsp]
    mov rsp, rbp
    pop rbp
    ret

global LoadGDT  ; void __attribute__((sysv_abi)) LoadGDT(uint16_t limit, uint64_t offset);
LoadGDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10
    mov [rsp], di  ; limit
    mov [rsp + 2], rsi  ; offset
    lgdt [rsp]
    mov rsp, rbp
    pop rbp
    ret

; Set SS and CS register
; - A smart way to set CS while not effecting execution flow;
; - CS and RIP need to be set at the same time by doing a far jump or far return;
; - With a far return, function parameters can be passed more intuitively (i.e., without using a "hack" like the haribote OS);
; - SS <- Stack Segment Selector; SS can be loaded explicitly (Intel SDM)
; - CS <- Code Segment Selector; CS cannot be loaded explicitly
; - In 64-Bit Mode, CS, DS, ES, SS are treated as if each segment base is 0, regardless of the value of the associated segment descriptor base (Intel SDM)
; - This creates a flat address space for code, data, and stack. Except FS and GS, which may be used as additional base registers in linear address calculations (Intel SDM)
; - Though generally disable, the segment selectors can still be used by applications running in compatibility mode
global SetCSSS  ; void __attribute__((sysv_abi)) SetCSSS(uint16_t cs, uint16_t ss);
SetCSSS:
    push rbp
    mov rbp, rsp

    mov ss, si  ; Set the SS register
    mov rax, .next ; the "Checkpoint"; Save the 64 bit address;
    push rdi    ; CS
    push rax    ; RIP
    o64 retf    ; Set RIP instead of EIP when "far return"; 48 cb rex.W retf (i.e., "REX prefix" syntax)
                ; `ret` instruction pops an address as EIP, if `retf` (return far), pops an additional address as CS; In 64-bit mode, the default operation size of the `ret` is 64bits, however, `retf` by default uses 32bits (Intel SDM)
.next:		; Resume original execution
    mov rsp, rbp
    pop rbp
    ret

global SetDSAll  ; void SetDSAll(uint16_t value);
SetDSAll:
    mov ds, di
    mov es, di
    mov fs, di
    mov gs, di
    ret

global SetCR3  ; void SetCR3(uint64_t value);
SetCR3:
; Enter Long Mode (Seems not need since UEFI start from 64 bit)
   ; ; disable paging
   ; mov rbx, cr0
   ; and rbx, ~(1 << 31)
   ; mov cr0, rbx
   ; ; Enable PAE
   ; mov rdx, cr4
   ; or rdx, (1 << 5)
   ; mov cr4, rdx
   ; ; Set LME (long mode enable)
   ; mov rcx, 0xC0000080
   ; rdmsr
   ; or  rax, (1 << 8)
   ; wrmsr

    mov cr3, rdi
;; Enable paging (and protected mode, if it isn't already active)
;    or rbx, (1 << 31) | (1 << 0)
;    mov cr0, rbx

    ret

extern kernel_main_stack
extern KernelMainNewStack

global _KernelMain
_KernelMain:
    mov rsp, kernel_main_stack + 1024 * 1024
    call KernelMainNewStack
.fin:
    hlt
    jmp .fin
