ENTRY(_start)

SECTIONS
{
	. = 0x0;

	.text : {
		*(.text*)
	}

	.rodata : ALIGN(8) {
		*(.rodata*)
	}

	.data : ALIGN(8) {
		*(.data*)
	}

	.bss : ALIGN(8) {
		*(.bss*)
	}
}