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
#include "../common/system_regs.h"
#include "../common/tegra30_uart.h"
#include "../common/printf.h"
//#include "../common/mmu_dump.h"


#define _MEM(addr) *(volatile uint32_t *)(addr)
#define mem_read(addr) _MEM(addr)
#define mem_write(addr, value) _MEM(addr) = value
#define mem_clear(base, value) _MEM(addr) &= ~value
#define mem_set(base, value) _MEM(addr) |= value

// adopted from kernel code. thanks Leander :)
struct parking_protocol_mailbox {
	uint32_t cpu_id;
	uint32_t reserved;
	uint64_t entry_point; // keep at 64Bit to keep cpu_mailbox_entry aligned
};

struct cpu_mailbox_entry {
	struct parking_protocol_mailbox *mailbox;
	uint32_t mailbox_addr;
	uint8_t version;
	uint8_t gic_cpu_id;
};


void start_secondary_core(int cpu);

//void dump_important_stuff();

extern void lock_mutex(void* mutex);
extern void unlock_mutex(void* mutex);


// Context switch from Non-Secure to Secure.
// Need to synchronize everything
// This is inserted before main() in the binary
asm (	"isb\n"
	"dsb\n");

void main()
{
	volatile uint32_t reg = 0;
	(void)reg; // no more warning pls
	//printf("-----------------------------------------------\r\n");
	//printf("--- Welcome to the otherside aka payload :) ---\r\n");
	//printf("-----------------------------------------------\r\n");

	reg = get_mpidr();

	printf("mpidr: 0x%08x\r\n", reg);
	printf("I am Core %d\r\n", reg & 0x3);

	// Switch to Secure World
	clear_ns();

	// GIC: change all interrupts to secure.
	uint32_t *ID_base = (uint32_t*)0x50041080;
	for (int i = 0; i < 6; i++) {
		ID_base[i] = 0;
	}
	
	//printf("Core: %d\r\n", core);
	if (reg & 0x3){
		printf("I am a secondary.\r\n");
		printf("I am not allowed to do the funny things. Bye :(\r\n");
		
		// Check if we are in Monitor mode
		reg = get_processor_mode();
		printf("Current Mode: %x\r\n", reg);
		
		reg = get_ns();
		printf("NS: %x; %s\r\n", reg, reg == 0 ? "Secure" : "Non-Secure");
		
		clear_ns();
		
		reg = get_ns();
		printf("NS: %x; %s\r\n", reg, reg == 0 ? "Secure" : "Non-Secure");
		
		// Check if Security Related Registers can be written
		//reg = get_SB_PFCFG_0();
		//printf("CP15SDISABLE: %08x\r\n", reg);
		
		// CP15SDISABLE & CFGSDISABLE
		//*((uint32_t*)0x6000C208) |= (0 << 4) | (0 << 5);

		//reg = get_SB_PFCFG_0();
		//printf("CP15SDISABLE: %08x\r\n", reg);
		
		// put me to sleep. I shouldn't do anything now.
		while(1); // good night. Burn hot
	} else {
		// Core0 will jump to Uboot.
		// continue execution to disable SMMU and stuff
		//printf("I am special :)\r\n");
		
		// Disable the MMU; here? secondaries shouldn't have MMU enabled
		disable_mmu();
		
		// Disable the SMMU
		*((uint32_t*)0x7000f010) = 0;
		
		printf("I am Core '0' and I will start Uboot now. Have fun\r\n");
	
		// JUMP TO UBOOT
		void (*uboot)(void) = (void (*)())0x84000000U;
		uboot();
		
		
		// optional stuff
		
		// Dump the flowcontroller
		// Might contain some hints about SMP problem
		/*
		uint32_t *flow_controller = (uint32_t*) 0x60007000U;
		
		for (int i = 0; i < 12; i++) {
			printf("fc%d:0x%08x\r\n", i, *(flow_controller+i));
		}
		*/
	}
	
	printf("Why are we here??? Something went wrong :(\r\n");
	while(1);
}
