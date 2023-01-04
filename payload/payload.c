// based on ipatch_rcm_sample.c provided by ktemkin (https://gist.github.com/ktemkin/825d5f4316f63a7c11ea851a2022415a)
// unmodified copy of original source can also be found at https://github.com/tofurky/tegra30_debrick/payload/ipatch_rcm_sample.c
// ipatch_word(), unipatch_word(), dump_word(), and dump_byte() are more or less unmodified.
// clock/uart initialization and offsets have been consolidated and modified for tegra30

// begin original header
/**
 * Proof of Concept Payload
 * prints out SBK and then enables non-secure RCM 
 *
 * (some code based on coreboot)
 * ~ktemkin
 */
// end original header

#include <stdint.h>
#include "system_regs.h"
#include "tegra30_uart.h"
#include "printf.h"
#include "mmu_dump.h"

void dump_important_stuff();

void main()
{
	volatile uint32_t reg;
	printf("-----------------------------------------------\r\n");
	printf("--- Welcome to the otherside aka payload :) ---\r\n");
	printf("-----------------------------------------------");
	
	// Check if we are in Monitor mode
	reg = get_processor_mode();
	printf("Current Mode: %x\r\n", reg);
	
	// Check if Security Related Registers can be written
	reg = get_cp15sdisable();
	printf("CP15SDISABLE: %x\r\n", reg);
	
	*((uint32_t*)0x6000C208) |= (1 << 4);
	
	reg = get_cp15sdisable();
	printf("CP15SDISABLE: %x\r\n", reg);


	dump_important_stuff();

	clear_ns();
	
	dump_important_stuff();
	
	reg = get_ns();
	printf("NS: %x; %s\r\n", reg, reg == 0 ? "Secure" : "Non-Secure");
	
	// JUMP TO UBOOT
	reg = *(uint32_t*)0x81000000U;
	printf("4Byte of 0x81000000: %x\r\n", reg);

	printf("Finished executing payload. bye :)\r\n");
	
	void (*uboot)(void) = (void (*)())0x81000000U;
	uboot();

	printf("Why are we here????\r\n");
	
	while(1);
}

void dump_important_stuff() {
	volatile uint32_t reg;

	reg = get_ns();
	printf("NS: %x; %s\r\n", reg, reg == 0 ? "Secure" : "Non-Secure");

	reg = get_dacr();
	printf("DACR: %x\r\n", reg);

	//set_dacr(0xFFFFFFFF);

	//reg = get_dacr();
	//printf("DACR: %x\r\n", reg);

	printf("---------------- START DUMP MMU-------------------\r\n");
	_start();
	printf("---------------- END DUMP MMU-------------------\r\n");
}
