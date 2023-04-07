ENTRY(_start)	/* The entry in the "kernel.o" */
OUTPUT_FORMAT(binary)
SECTIONS
{
	. = 1M;  /* assume kernel.o is loaded at position 1M */
	.text : ALIGN(4096)
	{
		*(.text);
	}
	.rodata : ALIGN(4096)
	{
		*(.rodata);
	}
	.data : ALIGN(4096)
	{
		*(.data);
	}
	.bss : ALIGN(4096)
	{
		*(COMMON);
		*(.bss);
	}
	.asm : ALIGN(4096)
	{
		*(.asm);
	}
}

