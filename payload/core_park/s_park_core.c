/*
 * Secondary Core parking payload. Should be executed in Secure World.
 */
#include <stdint.h>

uint32_t get_mpidr();

#define mailbox __attribute__ ((section(".mailbox")))

uint32_t mailbox cpu_id = 0xFFFFFFFF;
uint32_t mailbox entry_point = 0x00000000;

extern uint32_t cpu_id;


__attribute__ ((section(".text"))) void main()
{
	uint32_t entry;
	
	// Eventual Secure set up code here?
	// disable MMU? etc
	
	while(1) {
		if (((get_mpidr() & 0x03) == cpu_id) && (entry_point != 0x00000000)) {
			entry = entry_point;
			entry_point = 0;
			void (*kernel_startup)(void) = (void (*)())(uint32_t)entry;
			kernel_startup();
		}
		
		asm __volatile__ ("wfi");
	}
}

uint32_t get_mpidr() {
	uint32_t res;
	asm volatile ("MRC p15, 0, %0, c0, c0, 5" : "=r" (res) );
	return res;
}
