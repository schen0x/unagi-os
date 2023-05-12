[BITS 16]
[ORG 0x8200]

; PROTECTED_MODE_DATA_SEGMENT_SELECTOR equ 1000b		; Index 1, no flags set; The second entry in the GDTR
; PROTECTED_MODE_CODE_SEGMENT_SELECTOR equ 10000b		; Index 2, no flags set; The first entry in the GDTR
DATA_SEG equ GDT_DATA - GDT_START
CODE_SEG equ GDT_CODE - GDT_START			; This also calculates the offset. Because each Segment Descriptor is 64 bits or 8 bytes. Offset * 8 equals to the segment selector with no flag.

BOOT_DISK_INT13H_DRIVE_TYPE equ 80h			; 1st hard disk
LOAD_ADDRESS_NEXT_SECTOR_ES equ 820h			; ES * 0x10h == 0x8200


GDTR0 equ 0x0fe8					; **GDTR

VBE_MODE EQU 0x105					; VESA (105h) 1024×768 256 color
; VGA/VBE BOOT INFO
CYLS equ 0x0ff0						; ? OS_BOOT_BOOTINFO_ADDRESS?
LEDS equ 0x0ff1
VMODE equ 0x0ff2					; ?
SCRNX equ 0x0ff4					; Screen Resolution X
SCRNY equ 0x0ff6					; Screen Resolution Y
VRAM equ 0x0ff8						; Graphic Buffer Start Address


NEXT_SECTORS:
VESA_INIT:						; Select video mode
    ; Check if VBE is supported, if not, fallback to 320x200 256color
    mov ax, 0x9000					; Conventional memory
    mov ES, ax
    mov DI, 0
    mov ax, 0x4f00					; FUNCTION: Get VESA BIOS information; Return 0x4f00 if success; If success, ES:DI contains VESA BIOS information block
    int 0x10
    cmp ax, 0x004f
    jne .scrn320

    ; Check VBE version
    mov ax, [ES:DI + 4]					; VBE_INFO_STRUCTURE->(uint16_t)version
    cmp ax, 0x0200
    jb .scrn320						; if less than VESA 2.0, use fallback

    ; Check supported VBE mode
    mov cx, VBE_MODE
    mov ax, 0x4f01					; FUNCTION: Get VESA mode information; Input: CX = VESA mode number from the video modes array; Input: ES:DI = Segment:Offset pointer of where to store the VESA Mode Information Structure; Output: AX = 0x004F on success, other values indicate a BIOS error or a mode-not-supported error.
    int 0x10
    cmp ax, 0x004f
    jne .scrn320

    ; Select VBE mode
    mov ax, 0x4f02					; FUNCTION: Select VESA video modes
    mov bx, VBE_MODE | 0x4000				; | 0x4000 use linear frame buffer; VESA (mode 103h) 800x600 256color; (105h) 1024×768 256 color; (107h) 1280x1024 256color
    int 0x10
    mov BYTE [VMODE], 8					; ?
    mov ax, [ES:DI+0x12]
    mov word [SCRNX], ax
    mov ax, [ES:DI+0x14]
    mov word [SCRNY], ax
    mov dword eax, [ES:DI+0x28]				; VBE_MODE_INFO->(uint32_t)framebuffer
    mov dword [VRAM], eax 				; VBE_MODE_INFO->(uint32_t)framebuffer

    jmp KEYBOARD_STATUS

.scrn320:
    ; Fallback to VGA
    mov ah, 0
    mov al, 0x13					; VGA 300x200 256 color
    int 0x10
    mov byte [VMODE], 8
    mov word [SCRNX], 320
    mov word [SCRNY], 200
    mov dword [VRAM], 0x000a0000

    jmp KEYBOARD_STATUS


KEYBOARD_STATUS:
    ; Get keyboard status
    mov ah, 0x02
    int 0x16						; keyboard BIOS
    mov [LEDS], al


LOAD_PROTECTED:
.load_protected_prep:
    cli
    mov ax, 0x00
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00					; The stack grows downwards in x86 systems. So [0-0x7c00] is the stack, to prevent the stack from overwriting the bootloader itself.
.load_gdtr:
    mov DWORD [GDTR0], GDT_DESCRIPTOR			; Save the *GDTR0
    lgdt[GDT_DESCRIPTOR]				; load GDT with GDT_DESCRIPTOR (GDTR)
.enable_a20_line:
    in al, 0x92
    test al, 2
    jnz .enable_a20_line_end
    or al, 2
    and al, 0xFE
    out 0x92, al
.enable_a20_line_end:
.enable_protected_mode:
    mov eax, cr0					; Prepare to set PE (Protection Enable) bit in CR0 (Control Register 0)
    or al, 1						; Prepare to set PE bit in CR0
    mov cr0, eax					; Set PE (Protection Enable) bit in CR0 (Control Register 0)
    jmp CODE_SEG:LOAD32					; load the 32 bits code into memory and jump to it

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; GDT && GDTR ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GDT_START:
GDT_NULL:						; GDT Entry 0
    dd 0						; 4 bytes
    dd 0

GDT_DATA:						; DS, SS, ES, FS, GS
    dw 0xffff						; Segment Limit, first 0-15 bits, 0xffff is 256MB in 4kb page
    dw 0						; Base Address 15:00
    db 0						; Base Address 23:16
    db 0x92						; Access Byte for Kernel Mode Data Segment
    db 11001111b					; Flags and Limit; Limit 0xfffff => 4GB memory
    db 0						; Base 31:24

GDT_CODE:						; CS should point to this
    dw 0xffff						; Segment Limit, first 0-15 bits, 0xffff is 256MB in 4kb page
    dw 0						; Base Address 15:00
    db 0						; Base Address 23:16
    db 0x9a						; Common Access Byte 0x9a for Kernel Mode Code Segment
    db 11000111b					; Flags and Limit; Limit 0x7ffff * 4KB => 2GB
    db 0						; Base 31:24

GDT_END:
GDT_DESCRIPTOR:						; The GDTR
    dw (GDT_END - GDT_START - 1)			; The size of the table in bytes - 1; because max GDT_Size can be 65536, 1 byte more than 0xffff
    dd GDT_START					; Offset, 4 bytes in 32 bit mode

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 32 BIT START ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; PROTECTED_MODE ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;load kernel.asm and kernel.c to 0x100_000 (1M) ;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
[BITS 32]
LOAD32:
    push 0x00100000					; dst address when loading, edi
    push 0xf0						; Total sectors to load, max 0xff, ecx
    push 8						; VOLATILE; The starting sector to load; It's an lba number, hence 0 indexed. Two boot sectors have 4096 bytes in total, 4096 / 512 == 8, so _start is in the 9th sector;
    call ATA_LBA_READ
    add esp, 12
.load32_kernel_prep:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax						; TODO why es-ss set to DATA_SEG?
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp
.load32_kernel:
    jmp CODE_SEG:0x00100000				; jump to 1MB


ATA_LBA_READ:
    push ebp
    mov ebp, esp

    mov eax, 0
    mov es, eax

    xor ecx, ecx
    mov ecx, [ebp + 12]					; total sectors to load, max 0xff, ecx

    xor edi, edi
    mov edi, [ebp + 16]					; dst address when loading, edi

    mov ebx, [ebp + 8]					; The Logic Block Address (LBA)
    mov eax, ebx
    ; Send the highest 8 bits of the lba to hard disk controller
    shr eax, 24
    or eax, 0b01000000					; (28 bit PIO), select lba mode and the master drive
    mov dx, 0x1F6					; the destined port
    out dx, al
    ; Finished sending the highest 8 bits of the lba
    mov dx, 0x1F1					; the destined port
    mov al, 0
    out dx, al	; io wait

    ; Send the total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ; Finished sending the total bits to read

    ; Send lower 8 bits of the LBA
    mov eax, ebx					; Restore the backup LBA
    mov dx, 0x1F3
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send next 8 bits of the LBA
    mov eax, ebx					; Restore the backup LBA
    shr eax, 8
    mov dx, 0x1F4
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send the upper 16 bits of the LBA
    mov eax, ebx					; Restore the backup LBA
    shr eax, 16
    mov dx, 0x1F5
    out dx, al
    ; Finished sending the upper 16 bits of the LBA

    ; Send the "READ SECTORS" command (0x20)
    mov al, 0x20
    mov dx, 0x1f7
    out dx, al

; Read all sectors into memory
.next_sector:
    push ecx

; Checking if we need to read
.try_again:						; wait until disk controller ready
    mov dx, 0x1f7
    in al, dx
    test al, 8						; magic, ata status ready
    jz .try_again

; Read 256 words at a time
    mov ecx, 256					; ecx is the counter of rep
    mov dx, 0x1F0
    rep insw						; Read one word, repeat 256 times; Input doubleword from IO port specified in dx, into memory es:(e)di
    pop ecx						; Restore the ecx
    loop .next_sector					; Loop and decrement ecx by 1
    ; End of reading sectors to read

    pop ebp
    ret

times (4096 - 512) - ($ - $$) db 0
