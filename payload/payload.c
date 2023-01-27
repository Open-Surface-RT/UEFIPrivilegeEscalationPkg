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


#define _MEM(addr) *(volatile uint32_t *)(addr)
#define mem_read(addr) _MEM(addr)
#define mem_write(addr, value) _MEM(addr) = value
#define mem_clear(base, value) _R_MEMEG(addr) &= ~value
#define mem_set(base, value) _RE_MEMG(addr) |= value

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
	//reg = get_SB_PFCFG_0();
	//printf("CP15SDISABLE: %08x\r\n", reg);
	
	// CP15SDISABLE & CFGSDISABLE
	//*((uint32_t*)0x6000C208) |= (0 << 4) | (0 << 5);

	//reg = get_SB_PFCFG_0();
	//printf("CP15SDISABLE: %08x\r\n", reg);
	
	// bring up secondary cores
	// No idea if this can work.

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
	
	// MC_SMMU_CONFIG_0 = h10
	// MC 7000:f000
	*((uint32_t*)0x7000f010) = 0;
	
	clear_ns();
	
	disable_mmu();
	
	
	
	// MULTICORE ADVENTURE
	reg = get_mpidr();
	printf("mpidr: 0x%08x\r\n", reg);
	
	uint32_t core = reg & 0x03;
	printf("Core: %d\r\n", core);
	if (core == 0){
		printf("Let's start the secondary :)\r\n");
		start_secondary_core(1);
		while (1); // love you too.
	} else {
		
		printf("i am core: %d\r\n", core);
		printf("i am core: %d\r\n", core);
		printf("i am core: %d\r\n", core);
		printf("i am core: %d\r\n", core);
		printf("i am core: %d\r\n", core);
		while(1); // good night. Burn hot
	}
	
	// ADVENTURE ends here
	
	
	
	uint32_t *ID_base = (uint32_t*)0x50041080;
	
	for (int i = 0; i < 6; i++) {
		//printf("ID_sec: 0x%08x\r\n", ID_base[i]);
		//if (i == 0) continue;
		
		ID_base[i] = 0;
		//printf("ID_sec: 0x%08x\r\n", ID_base[i]);
		//printf("-------\r\n");
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


static struct cpu_mailbox_entry cpu_mailbox_entries[4];

// removed 'unsigned int cpu' from parameter list and changed return type to void
static void acpi_parking_protocol_cpu_init(void)
{
	printf("%s: has been called. Hardcoding MADT table for Surface RT.\r\n", __func__);

	cpu_mailbox_entries[0].gic_cpu_id = 0;
	cpu_mailbox_entries[0].version = 1;
	cpu_mailbox_entries[0].mailbox_addr = 0x82001000;
	cpu_mailbox_entries[0].mailbox = (struct parking_protocol_mailbox*)(0x82001000U);

	cpu_mailbox_entries[1].gic_cpu_id = 1;
	cpu_mailbox_entries[1].version = 1;
	cpu_mailbox_entries[1].mailbox_addr = 0x82002000;
	cpu_mailbox_entries[1].mailbox = (struct parking_protocol_mailbox*)(0x82002000U);

	cpu_mailbox_entries[2].gic_cpu_id = 2;
	cpu_mailbox_entries[2].version = 1;
	cpu_mailbox_entries[2].mailbox_addr = 0x82003000;
	cpu_mailbox_entries[2].mailbox = (struct parking_protocol_mailbox*)(0x82003000U);

	cpu_mailbox_entries[3].gic_cpu_id = 3;
	cpu_mailbox_entries[3].version = 1;
	cpu_mailbox_entries[3].mailbox_addr = 0x82004000;
	cpu_mailbox_entries[3].mailbox = (struct parking_protocol_mailbox*)(0x82004000U);
}

void start_secondary_core(int cpu) {
	acpi_parking_protocol_cpu_init();
	
	printf("Let's goooo\r\n");
	
	printf("mailbox_address: %p\r\n", &cpu_mailbox_entries[cpu].mailbox->cpu_id);
	
	printf("mailbox_value: %08x\r\n", *((uint32_t*)0x82002000U));
	
	uint32_t cpu_id = *((uint32_t*)0x82002000U); //mem_read(cpu_mailbox_entries[cpu].mailbox->cpu_id);
	printf("cpu: %d\r\n", cpu);
	printf("cpu_id: %d\r\n", cpu_id);
	
	if (cpu_id != ~0U) {
		printf("something wrong\r\n");
	}
	
	// Let the secondary core use the payload loaded by UEFI.
	printf("entry_write: %p\r\n", (uint32_t)(&cpu_mailbox_entries[cpu].mailbox->entry_point));
	mem_write((uint32_t)(&cpu_mailbox_entries[cpu].mailbox->entry_point), 0x83800000U);
	
	printf("cpu_write: %p\r\n", (uint32_t)(&cpu_mailbox_entries[cpu].mailbox->cpu_id));
	mem_write((uint32_t)(&cpu_mailbox_entries[cpu].mailbox->cpu_id), cpu); 
	
	// Interrupt magic. 
	// interrupt according to ACPI PP 0x00fe0000
	// reg: 0xf00
	// base: 0x50041000
	
	printf("mailbox_cpu_id: %08x\r\n", *((uint32_t*)0x82002000U));
	printf("mailbox_entry: %08x\r\n", *((uint32_t*)0x8200200bU));
	
	set_ns();
	
	printf("now the interrupt\r\n");
	mem_write(0x50041f00U, 0x00fe0000U);
	printf("interrupt done?!\r\n");
	
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
