/* based on ../fusee-launcher/dump-sbk-via-usb.lds */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(startup)
SECTIONS
{
	.text.entry : {
		__code_start = .;
		*(.text.entry)
		. = ALIGN(4);
	}

	.text : {
		*(.text)
		. = ALIGN(4);
	}

	/* always end on a word boundary for our copy */
	. = ALIGN(4);

	/*/DISCARD/ : { *(.dynstr*) }
	/DISCARD/ : { *(.dynamic*) }
	/DISCARD/ : { *(.plt*) }
	/DISCARD/ : { *(.interp*) }
	/DISCARD/ : { *(.gnu*) }
	/DISCARD/ : { *(.data*) }*/

	.pagetable 0x80010000 (NOLOAD):
    {
        __pagetable_start = .;
        . = . + 0x4000;
    }

	.uboot 0x84000000 (NOLOAD):
    {
        __uboot_base = .;
    }
}