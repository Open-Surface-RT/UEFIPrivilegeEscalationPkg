#include <stdint.h>
#include "system_regs.h"

#define SB 0x6000C000
#define SB_PFCFG_0 0x8

uint32_t get_processor_mode() {
	uint32_t res;
	asm volatile ("MRS %0, CPSR" : "=r" (res) );
	return res & 0b11111;
}

uint32_t get_SB_PFCFG_0() {
	return *((uint32_t*)0x6000C208);
}

uint32_t get_cp15sdisable() {
	return (get_SB_PFCFG_0() & (1 << 4)) >> 4;
}

uint32_t get_tbbr0() {
	uint32_t res;
	asm volatile ("MRC p15, 0, %0, c2, c0, 0" : "=r" (res) );
	return res;
}

void set_tbbr0(uint32_t val) {
	asm volatile ("MCR p15, 0, %0, c2, c0, 0" : : "r" (val) );
}

uint32_t get_dacr() {
	uint32_t res;
	asm volatile ("MRC p15, 0, %0, c3, c0, 0" : "=r" (res) );
	return res;
}

void set_dacr(uint32_t val) {
	asm volatile (	"MCR p15, 0, %0, c3, c0, 0\n"
			"ISB SY" : : "r" (val) );
}

uint32_t get_ns() {
	uint32_t res;
	asm volatile ("MRC p15, 0, %0 , c1, c1 ,0" : "=r" (res) );
	return res & 0x01;
}

void set_ns() {
	asm volatile (	"MRC p15, 0, r0, c1, c1 ,0 \n"
			"orr r0, r0, #0x01 \n"
			"MCR p15, 0 , r0, c1 ,c1 ,0 \n"
			"ISB SY \n"
			);
}
void clear_ns(uint32_t val) {
	asm volatile (	"MRC p15, 0, r0, c1, c1, 0 \n"
			"bic r0, r0, #0x01 \n"
			"MCR p15, 0 , r0, c1 ,c1, 0 \n"
			"ISB SY \n"
			);
}

void disable_mmu() {
	asm volatile (	"mrc   p15, 0, r0, c1, c0, 0 \n"           // Get control register
			"bic   r0, r0, #0x1 \n"             	    // Disable MMU
			"mcr   p15, 0, r0, c1, c0, 0 \n"           // Write control register
			"dsb\n"
			"isb\n"
			);
}

uint32_t get_mpidr() {
	uint32_t res;
	asm volatile ("MRC p15, 0, %0, c0, c0, 5" : "=r" (res) );
	return res;
}
