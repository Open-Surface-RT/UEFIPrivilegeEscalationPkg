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

#define _MEM(addr) *(volatile uint32_t *)(addr)
#define mem_read(addr) _MEM(addr)
#define mem_write(addr, value) _MEM(addr) = value
#define mem_clear(base, value) _MEM(addr) &= ~value
#define mem_set(base, value) _MEM(addr) |= value

asm("ldr sp, =0x88008000U");

void main()
{
	// Context switch from Non-Secure to Secure.
	// Need to synchronize everything
	asm volatile (	"ISB SY \n"
			"DSB SY \n"
	);
	
	// Indicate that secondary switched to Secure
	printf("Hello There ;)\r\n");
	for (int i = 0; i < 2; i++) {
		mem_write((uint32_t)0x88008000U, 11223344);
		mem_write((uint32_t)0x88008000U, 55667788);
	}

	while(1);
}
