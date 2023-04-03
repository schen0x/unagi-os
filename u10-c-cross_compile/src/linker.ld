ENTRY(_start)	/* The entry in the "kernel.o" */
OUTPUT_FORMAT(binary)
SECTIONS
{
	. = 1M;  /* assume kernel.o is loaded at position 1M */
	.text :
	{
		*(.text)
	}

	.rodata :
	{
		*(.rodata)
	}

	.data :
	{
		*(.data)
	}

	.bss :
	{
		*(COMMON)
		*(.bss)
	}
}
