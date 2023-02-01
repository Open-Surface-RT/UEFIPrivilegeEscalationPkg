/* based on ../fusee-launcher/dump-sbk-via-usb.lds */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(main)

STACK_SIZE = 0x400; /* 1 kB */

SECTIONS
{
	. = 0x82002820;
	.text : {
		*(.text)
	}
	.stack (NOLOAD) :
	{
		. = ALIGN(8);
		. = . + STACK_SIZE;
		. = ALIGN(8);
	}

	/* always end on a word boundary for our copy */
	. = ALIGN(4);

	/DISCARD/ : { *(.dynstr*) }
	/DISCARD/ : { *(.dynamic*) }
	/DISCARD/ : { *(.plt*) }
	/DISCARD/ : { *(.interp*) }
	/DISCARD/ : { *(.gnu*) }
	/DISCARD/ : { *(.data*) }
}
