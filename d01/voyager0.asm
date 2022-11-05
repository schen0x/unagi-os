; FAT12 minium OS for a Floppy Disk

[BITS 16]						; Tells nasm this is 16 bit code
[ORG 0x7c00]						; Origin; tells nasm the code will be loaded to memory 0x0
; ORG 0x7c00
; DB	0xeb, 0x4e, 0x90
jmp short START
nop

    OEM_ID                db		"VOYAGER0"	; 8 bytes

    ; Extended BIOS Parameter Block (EBPB) (used by FAT12 and FAT16), START
    ; DOS 3.31 BPB, START
    ; DOS 2.0 BPB, START
    BytesPerSector        dw		0x0200		; 512 bytes
    SectorsPerCluster     db 		1
    ReservedSectors       dw 		1
    TotalFATs             db 		2
    MaxRootEntries        dw 		224
    TotalLogicalSectors   dw 		2880
    MediaDescriptor       db 		0xF0		; 0xF0 floopy; 0xF8 fixed disk
    LogicalSectorsPerFAT  dw 		9
    ; DOS 2.0 BPB, END
    SectorsPerTrack       dw 		18
    SectorsPerHead        dw 		0x0002
    HiddenSectors         dd 		0
    TotalLogicalSectors2  dd 		2880		; @\x020
    ; DOS 3.31 BPB, END

    PhysicalDriveNumber   db		0		; Physical drive number (0x00 for (first) removable media, 0x80 for (first) fixed disk as per INT 13h)
    Reserve               db            0
    ExtendedBootSignature db            0x29		; 0x29 to indicate EBPB
    VolumeID              dd		0xffffffff	; some serial number
    PartitionVolumeLabel  db            "VOYAGER0DSK"	; 11 bytes
    FileSystemType        db		"FAT12   "      ; @\x036; 8 bytes
    ; Extended BIOS Parameter Block (EBPB), END
    Reserve2              RESB		18

START:
    mov	ax, 0
    mov	ss, ax
    mov	sp, 0x7c00
    mov	ds, ax
    mov	es, ax
    mov	si, MSG

PUTLOOP:
    mov	al,[si]
    add	si,1
    cmp	al,0
    je	FIN
    mov	ah, 0x0e	; print 1 char
    mov bx, 15		; colorcode
    int 0x10		;
    JMP PUTLOOP

FIN:
    hlt
    jmp FIN


MSG:
    db 0x0a, 0x0a
    db "VOYAGER0: Greeting, human."
    db 0x0a
    db 0

    ; ??

    ; resb 0x1fe-$
    resb 0x1fe - ($-$$)
    ; TIMES 0x1fe-($-$$) DB 0
    db 0x55, 0xaa

    db 0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
    resb 4600
    db 0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
    resb 1469432

