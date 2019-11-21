SECTIONS
{
	. = LOADADDR;
	. = ALIGN(4);
	.text : {
		KEEP(*(.entry))
		*(.text.startup)
		*(.text)
	}
	.rodata : { *(.rodata*)}
	.got.plt : { *(.got.plt)}
	.data : { *(.data)}
	.bss : {
		*(.bss)
		*(.bss*)
    	*(COMMON)
	}

	/DISCARD/ : {*(*)}
}
