#ifndef SYSTEM_REGS_H
#define SYSTEM_REGS_H

uint32_t get_processor_mode();
uint32_t get_cp15sdisable();
uint32_t get_tbbr0();

void set_tbbr0(uint32_t val);

uint32_t get_dacr();
void set_dacr(uint32_t val);

uint32_t get_ns();
void set_ns();
void clear_ns();

void disable_mmu();
uint32_t get_SB_PFCFG_0();
uint32_t get_mpidr();

#endif
