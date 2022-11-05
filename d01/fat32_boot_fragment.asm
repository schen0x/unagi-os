; custom_os
; TAB=4

DB 0xeb, 0x4e, 0x90
DB "LUMOS0.1" ; 8 bytes OEM OS Name
; Full DOS 7.1 EBPB, for FAT32, START
; DOS 3.31 BPB, START
; DOS 2.0 BPB, START
DW 512     ; \x00B; Bytes per logical sector; the most common value is 512. In mordern Linux can be 32KB
DB 1       ; Logical sectors per cluster. Allowed value: 1,2,4,8,16,32,64,128.
DW 32      ; book: 2; Count of reserved logical sectors. The number of logical sectors before the first FAT in the file system image. At least 1 for this sector, usually 32 for FAT32 (to hold the extended boot sector, FS info sector and backup boot sectors).
DB 2       ; Number of File Allocation Tables. Almost always 2;
DW 0       ; \x011; book: 224; Maximum number of FAT12 or FAT16 root directory entries. 0 for FAT32 and Config at \x02C
DW 0       ; \x013; book: 2880; Total logical sectors. 0 for FAT32. (If zero, use 4 byte value at offset \x020)
DB 0xF8    ; Media descriptor. Book: 0xF0; 1440KB 3.5-inch floopy. 0xFB Fixed disk
DW 0       ; \x016 Logical sectors per File Allocation Table for FAT12/FAT16. FAT32 sets this to 0 and uses the 32-bit value at -> \x024 instead.
; DOS 2.0 BPB, END
DW 1       ; \x018; Book: 18; Physical sectors per track for disks with INT 13h CHS geometry, e.g., 18 for a "1.44 MB" (1440 KB) floppy. A zero entry indicates that this entry is reserved, but not used. A value of 0 may indicate LBA-only access, but may cause a divide-by-zero exception in some boot loaders, which can be avoided by storing a neutral value of 1 here, if no CHS geometry can be reasonably emulated.
DW 1        ; Book: 2; Number of heads for disks with INT 13h CHS geometry.
DD 0        ; Count of hidden sectors preceding the partition that contains this FAT volume.
DD 2880     ; \x020; Total logical sectors
; DOS 3.31 BPB, END
; TODO
DD 9        ; Logical sectors per file allocation table -> \x00B
RESB 2      ; Drive description / mirroring flags
RESB 2      ; Version
DD
DB
DB
RESB 12
DB
DB
DB
DD
DB ; \x047; 11 bytes
DB "FAT32   "



; FAT
