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
	asm volatile ("MCR p15, 0, %0, c3, c0, 0" : : "r" (val) );
}

uint32_t get_ns() {
	uint32_t res;
	asm volatile ("MRC p15, 0, %0 , c1, c1 ,0" : "=r" (res) );
	return res & 1;
}

void set_ns(uint32_t val) {
	asm volatile ("MCR p15, 0 , %0, c1 ,c1 ,0" : : "r" (val) );
}
