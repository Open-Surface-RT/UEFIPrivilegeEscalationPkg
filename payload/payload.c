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

void main()
{
	volatile uint32_t reg;
	char buffer[100];
	
	uart_print("Welcome to the otherside aka payload :)\r\n");
	
	reg = get_processor_mode();
	utoa(reg, buffer, 16);
	uart_print("Current Mode:");
	uart_print(buffer);
	uart_print("\r\n");
	
	reg = get_cp15sdisable();
	utoa(reg, buffer, 16);
	uart_print("CP15SDISABLE:");
	uart_print(buffer);
	uart_print("\r\n");
	
	*((uint32_t*)0x6000C208) |= (1 << 4);
	
	reg = get_cp15sdisable();
	utoa(reg, buffer, 16);
	uart_print("CP15SDISABLE:");
	uart_print(buffer);
	uart_print("\r\n");

	reg = get_tbbr0();
	utoa(reg, buffer, 16);
	uart_print("TTBR0:");
	uart_print(buffer);
	uart_print("\r\n");
/*
	set_tbbr0(0);
	
	reg = get_tbbr0();
	utoa(reg, buffer, 16);
	uart_print("TTBR0:");
	uart_print(buffer);
	uart_print("\r\n");
	

	reg = get_dacr();
	utoa(reg, buffer, 16);
	uart_print("DACR:");
	uart_print(buffer);
	uart_print("\r\n");
	
	set_dacr(0xFFFFFFFF);
	
	reg = get_dacr();
	utoa(reg, buffer, 16);
	uart_print("DACR:");
	uart_print(buffer);
	uart_print("\r\n");
*/
	reg = get_ns();
	utoa(reg, buffer, 16);
	uart_print("NS:");
	uart_print(buffer);
	uart_print("\r\n");
	
	
	uart_print("Finished executing payload.\r\n");

	// Dead loop. We don't want to execute random memory. 
	// Returning to UEFI Application doesn't work either. (at least right now)
	
	reg = *(uint32_t*)0x81000000U;
	utoa(reg, buffer, 16);
	uart_print("4Byte of 0x81000000:");
	uart_print(buffer);
	uart_print("\r\n");
	
	void (*uboot)(void) = (void (*)())0x81000000U;
	uboot();

	uart_print("Why are we here????\r\n");
	
	while(1);
}


