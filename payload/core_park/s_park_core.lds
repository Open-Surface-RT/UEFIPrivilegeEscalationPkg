/* based on ../fusee-launcher/dump-sbk-via-usb.lds */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(main)

MEMORY 
{
      /* Define each memory region */ 
      MAILBOX (rwx) : ORIGIN = 0x0, LENGTH = 0x1000 /* 4k */ 
      MAILBOX_CODE (rwx) : ORIGIN = 0x1000, LENGTH = 0x1000 /* 4k */ 
      
}

SECTIONS
{
	. = LOAD_ADDR;
	
	.mailbox_values :  {
		KEEP(*(.mailbox)) 
		. = ALIGN(0x1000);
	} > MAILBOX

	.text : {
		KEEP(*(.text))
		. = ALIGN(0x1000);
		
	} > MAILBOX_CODE =0x00

	/* always end on a word boundary for our copy */


	/DISCARD/ : { *(.dynstr*) }
	/DISCARD/ : { *(.dynamic*) }
	/DISCARD/ : { *(.plt*) }
	/DISCARD/ : { *(.interp*) }
	/DISCARD/ : { *(.gnu*) }
	/DISCARD/ : { *(.data*) }
}
