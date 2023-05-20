SECTION .text

global _farjmp:FUNCTION
_farjmp:	; void farjmp(uint32_t eip, uint16_t cs)
    JMP FAR[ESP + 4]	; a hack? "FF /5" "JMP m16:32" ==> jmp cs:eip; also eip is ignored
    ret

times 512-($ - $$) db 0

