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
	//printf("-----------------------------------------------\r\n");
	//printf("--- Welcome to the otherside aka payload :) ---\r\n");
	//printf("-----------------------------------------------\r\n");
	/*
	// Check if we are in Monitor mode
	reg = get_processor_mode();
	printf("Current Mode: %x\r\n", reg);
	
	reg = get_ns();
	printf("NS: %x; %s\r\n", reg, reg == 0 ? "Secure" : "Non-Secure");
	
	clear_ns();
	
	reg = get_ns();
	printf("NS: %x; %s\r\n", reg, reg == 0 ? "Secure" : "Non-Secure");
	*/
	// Check if Security Related Registers can be written
	reg = get_SB_PFCFG_0();
	printf("CP15SDISABLE: %08x\r\n", reg);
	
	// CP15SDISABLE & CFGSDISABLE
	*((uint32_t*)0x6000C208) |= (0 << 4) | (0 << 5);

	reg = get_SB_PFCFG_0();
	printf("CP15SDISABLE: %08x\r\n", reg);
	
//	MC_SMMU_CONFIG_0 = h10
//	MC 7000:f000
	*((uint32_t*)0x7000f010) = 0;
	
	// Dump the flowcontroller
	// Might contain some hints about SMP problem
	uint32_t *flow_controller = (uint32_t*) 0x60007000U;
	
	for (int i = 0; i < 12; i++) {
		printf("fc%d:0x%08x\r\n", i, *(flow_controller+i));
	}
	
	/*
	reg = get_cp15sdisable();
	printf("CP15SDISABLE: %x\r\n", reg);

	
	// Dump secure world stuff
	printf("---------------------------\r\n");
	printf("--- Secure World params ---\r\n");
	printf("---------------------------\r\n");
	//dump_important_stuff();

	reg = *(uint32_t*)0x84000000U;
	printf("4Byte of 0x84000000: %x\r\n", reg);
	if (reg == 0xFFFFFFFF) {
		printf("Can't access non-secure memory :(\r\n");
	} else {
		printf("Can access non-secure memory :)\r\n");
	}


	// Dump non-secure world stuff
	printf("-------------------------------\r\n");
	printf("--- Non-Secure World params ---\r\n");
	printf("-------------------------------\r\n");
	set_ns();
	
	reg = *(uint32_t*)0x84000000U;
	printf("4Byte of 0x84000000: %x\r\n", reg);
	if (reg == 0xFFFFFFFF) {
		printf("Can't access non-secure memory :(\r\n");
	} else {
		printf("Can access non-secure memory :)\r\n");
	}
	//dump_important_stuff();
	
	reg = get_ns();
	printf("NS: %x; %s\r\n", reg, reg == 0 ? "Secure" : "Non-Secure");
	*/
	
	printf("Payload bitch :)\r\n");
	
	clear_ns();
	
	disable_mmu();
	
	uint32_t *ID_base = (uint32_t*)0x50041080;
	
	for (int i = 0; i < 6; i++) {
		printf("ID_sec: 0x%08x\r\n", ID_base[i]);
		if (i == 0) continue;
		
		ID_base[i] = 0;
		printf("ID_sec: 0x%08x\r\n", ID_base[i]);
		printf("-------");
	}
	
	//reg = get_ns();
	//printf("NS: %x; %s\r\n", reg, reg == 0 ? "Secure" : "Non-Secure");
	
	// JUMP TO UBOOT
	//reg = *(uint32_t*)0x81000000U;
	//printf("4Byte of 0x81000000: %x\r\n", reg);

	printf("Finished executing payload. bye :)\r\n");
	
	
	//asm volatile ("ldr pc, =0x81000000" : : );
	
	void (*uboot)(void) = (void (*)())0x84000000U;
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
