#include "Include/Application.h"

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

static struct cpu_mailbox_entry cpu_mailbox_entries[4];

// removed 'unsigned int cpu' from parameter list and changed return type to void
void acpi_parking_protocol_cpu_init(void)
{
	//Print(L"%s: has been called. Hardcoding MADT table for Surface RT.\r\n", __func__);

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



void start_secondary_core(uint32_t cpu) {
	acpi_parking_protocol_cpu_init();

	//Print(L"Let's goooo\r\n");

	//Print(L"mailbox_address: %p\r\n", &cpu_mailbox_entries[cpu].mailbox->cpu_id);

	//Print(L"mailbox_value: %08x\r\n", *((uint32_t*)0x82002000U));

	uint32_t cpu_id = *((uint32_t*)0x82002000U); //mem_read(cpu_mailbox_entries[cpu].mailbox->cpu_id);
	//Print(L"cpu: %d\r\n", cpu);
	//Print(L"cpu_id: %d\r\n", cpu_id);

	if (cpu_id != ~0U) {
		Print(L"something wrong\r\n");
	}

	// Let the secondary core use the payload loaded by UEFI.
	//Print(L"entry_write: %p\r\n", (uint32_t)(&cpu_mailbox_entries[cpu].mailbox->entry_point));
	//mem_write((uint32_t)(&cpu_mailbox_entries[cpu].mailbox->entry_point), 0x83800000U);
	*((uint32_t*)0x82002008) = 0x8300A000;
	//Print(L"cpu_write: %p\r\n", (uint32_t)(&cpu_mailbox_entries[cpu].mailbox->cpu_id));
	//mem_write((uint32_t)(&cpu_mailbox_entries[cpu].mailbox->cpu_id), cpu);
	*((uint32_t*)0x82002000) = 0x00000001;
	// Interrupt magic.
	// interrupt according to ACPI PP 0x00fe0000
	// reg: 0xf00
	// base: 0x50041000

	//Print(L"mailbox_cpu_id: %08x\r\n", *((uint32_t*)0x82002000U));
	//Print(L"mailbox_entry: %08x\r\n", *((uint32_t*)0x82002008U));

	//set_ns();

	//Print(L"now the interrupt\r\n");
	//mem_write(0x50041f00U, 0x00fe0000U);
	asm volatile("dmb ishst\n");
	// use ArmGicSendSgiTo : ArmPkg/Drivers/ArmGiC/ArnGicLib.c
	// Does the same thing boot looks cooler ;)
	*((uint32_t*)0x50041f00U) = 0x00fe0000U;

	uart_print("Waiting for Mailbox to clear\r\n");
	while (mem_read(0x82002008)) {
		;
	}
	uart_print("Mailbox cleared\r\n");

}
