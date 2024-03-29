[BITS 32]

section .text

global paging_load_directory:function
global enable_paging:function

paging_load_directory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]
    mov cr3, eax
    pop ebp
    ret

enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax
    pop ebp
    ret

global _loadPageDirectory:function
_loadPageDirectory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]
    mov cr3, eax
    mov esp, ebp
    pop ebp
    ret

times 4096-($ - $$) db 0
