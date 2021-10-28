// Copyright (c) 2007-2017 Arm Limited (or its affiliates). All rights reserved.
// Copyright (c) 2021 - 2021, Leander Wollersberger. All rights reserved.
// This file takes reference from the startup_Cortex-A9_GCC example project from the ARM Developer Studio IDE.

.section .text.entry

// This is the entry point, it disables caches, MMU and branch prediction and then sets the stack pointer
.global startup
.type startup, function
startup:
        b startup_real

.section .text
.align 4

startup_real:
        bl set_cpsr

//----------------------------------------------------------------
// Disable caches, MMU and branch prediction in case they were left enabled from an earlier run
// This does not need to be done from a cold reset
//----------------------------------------------------------------

        MRC     p15, 0, r0, c1, c0, 0       // Read System Control Register
        BIC     r0, r0, #(0x1 << 12)        // Clear I bit 12 to disable I Cache
        BIC     r0, r0, #(0x1 <<  2)        // Clear C bit  2 to disable D Cache
        BIC     r0, r0, #0x1                // Clear M bit  0 to disable MMU
        BIC     r0, r0, #(0x1 << 11)        // Clear Z bit 11 to disable branch prediction
        MCR     p15, 0, r0, c1, c0, 0       // Write System Control Register
        ISB

//----------------------------------------------------------------
// Invalidate Data and Instruction TLBs and branch predictor
//----------------------------------------------------------------
	
        MOV     r0,#0
        MCR     p15, 0, r0, c8, c7, 0      // I-TLB and D-TLB invalidation
        MCR     p15, 0, r0, c7, c5, 6      // BPIALL - Invalidate entire branch predictor array

//----------------------------------------------------------------
// Cache Invalidation code for Cortex-A9
//----------------------------------------------------------------

        // Invalidate L1 Instruction Cache

        MRC     p15, 1, r0, c0, c0, 1      // Read Cache Level ID Register (CLIDR)
        TST     r0, #0x3                   // Harvard Cache?
        MOV     r0, #0                     // SBZ
        MCRNE   p15, 0, r0, c7, c5, 0      // ICIALLU - Invalidate instruction cache and flush branch target cache

        // Invalidate Data/Unified Caches

        MRC     p15, 1, r0, c0, c0, 1      // Read CLIDR
        ANDS    r3, r0, #0x07000000        // Extract coherency level
        MOV     r3, r3, LSR #23            // Total cache levels << 1
        BEQ     Finished                   // If 0, no need to clean

        MOV     r10, #0                    // R10 holds current cache level << 1
Loop1:
        ADD r2, r10, r10, LSR #1           // R2 holds cache "Set" position
        MOV     r1, r0, LSR r2             // Bottom 3 bits are the Cache-type for this level
        AND     r1, r1, #7                 // Isolate those lower 3 bits
        CMP     r1, #2
        BLT     Skip                       // No cache or only instruction cache at this level

        MCR     p15, 2, r10, c0, c0, 0     // Write the Cache Size selection register
        ISB                                // ISB to sync the change to the CacheSizeID reg
        MRC     p15, 1, r1, c0, c0, 0      // Reads current Cache Size ID register
        AND     r2, r1, #7                 // Extract the line length field
        ADD     r2, r2, #4                 // Add 4 for the line length offset (log2 16 bytes)
        LDR     r4, =0x3FF
        ANDS    r4, r4, r1, LSR #3         // R4 is the max number on the way size (right aligned)
        CLZ     r5, r4                     // R5 is the bit position of the way size increment
        LDR     r7, =0x7FFF
        ANDS    r7, r7, r1, LSR #13        // R7 is the max number of the index size (right aligned)

Loop2:
        MOV     r9, r4                     // R9 working copy of the max way size (right aligned)

Loop3:
        ORR     r11, r10, r9, LSL r5       // Factor in the Way number and cache number into R11
        ORR     r11, r11, r7, LSL r2       // Factor in the Set number
        MCR     p15, 0, r11, c7, c6, 2     // Invalidate by Set/Way
        SUBS    r9, r9, #1                 // Decrement the Way number
        BGE     Loop3
        SUBS    r7, r7, #1                 // Decrement the Set number
        BGE     Loop2
Skip:
        ADD     r10, r10, #2               // Increment the cache number
        CMP     r3, r10
        BGT     Loop1

Finished:

//----------------------------------------------------------------
// MMU Configuration
// Set translation table base
//----------------------------------------------------------------

        // Two translation tables are supported, TTBR0 and TTBR1
        // Configure translation table base (TTB) control register cp15,c2
        // to a value of all zeros, indicates we are using TTB register 0.

        MOV     r0,#0x0
        MCR     p15, 0, r0, c2, c0, 2

        // write the address of our page table base to TTB register 0
        LDR     r0,=__pagetable_start
        MOV     r1, #0x08                  // RGN=b01  (outer cacheable write-back cached, write allocate)
                                           // S=0      (translation table walk to non-shared memory)
        ORR     r1,r1,#0x40                // IRGN=b01 (inner cacheability for the translation table walk is Write-back Write-allocate)

        ORR     r0,r0,r1
        MCR     p15, 0, r0, c2, c0, 0


//----------------------------------------------------------------
// PAGE TABLE generation 

// Generate the page tables
// Build a flat translation table for the whole address space.
// ie: Create 4096 1MB sections from 0x000xxxxx to 0xFFFxxxxx


// 31                 20 19  18  17  16 15  14   12 11 10  9  8     5   4    3 2   1 0
// |section base address| 0  0  |nG| S |AP2|  TEX  |  AP | P | Domain | XN | C B | 1 0|
//
// Bits[31:20]   - Top 12 bits of VA is pointer into table
// nG[17]=0      - Non global, enables matching against ASID in the TLB when set.
// S[16]=0       - Indicates normal memory is shared when set.
// AP2[15]=0  
// AP[11:10]=11  - Configure for full read/write access in all modes
// TEX[14:12]=000
// CB[3:2]= 00   - Set attributes to Strongly-ordered memory.
//                 (except for the code segment descriptor, see below)
// IMPP[9]=0     - Ignored
// Domain[5:8]=1111   - Set all pages to use domain 15
// XN[4]=1       - Execute never on Strongly-ordered memory
// THIS
// https://developer.arm.com/documentation/ddi0406/b/System-Level-Architecture/Virtual-Memory-System-Architecture--VMSA-/Translation-tables/Translation-table-entry-formats?lang=en
// https://developer.arm.com/documentation/ddi0406/b/System-Level-Architecture/Virtual-Memory-System-Architecture--VMSA-/Memory-access-control/The-Execute-Never--XN--attribute-and-instruction-prefetching?lang=en


// Bits[1:0]=10  - Indicate entry is a 1MB section
//----------------------------------------------------------------
        LDR     r0,=__pagetable_start
        LDR     r1,=0xfff                   // loop counter
        LDR     r2,=0b00000000000000000000110111100010

        // r0 contains the address of the translation table base
        // r1 is loop counter
        // r2 is level1 descriptor (bits 19:0)

        // use loop counter to create 4096 individual table entries.
        // this writes from address __pagetable_start +
        // offset 0x3FFC down to offset 0x0 in word steps (4 bytes)

init_ttb_1:
        ORR     r3, r2, r1, LSL#20          // R3 now contains full level1 descriptor to write
//        ORR     r3, r3, #0b0000000010000    // Set XN bit
        STR     r3, [r0, r1, LSL#2]         // Str table entry at TTB base + loopcount*4
        SUBS    r1, r1, #1                  // Decrement loop counter
        BPL     init_ttb_1

        // In this example, the 1MB section based at '__code_start' is setup specially as cacheable (write back mode).
        // TEX[14:12]=001 and CB[3:2]= 11, Outer and inner write back, write allocate normal memory.
        LDR     r1,=__code_start           // Base physical address of code segment
        LSR     r1, #20                    // Shift right to align to 1MB boundaries
        ORR     r3, r2, r1, LSL#20         // Setup the initial level1 descriptor again
        ORR     r3, r3, #0b0000000001100   // Set CB bits
        ORR     r3, r3, #0b1000000000000   // Set TEX bit 12
        STR     r3, [r0, r1, LSL#2]        // str table entry

        // Set uboot also to this
        // In this example, the 1MB section based at '__code_start' is setup specially as cacheable (write back mode).
        // TEX[14:12]=001 and CB[3:2]= 11, Outer and inner write back, write allocate normal memory.
        LDR     r1,=__uboot_base           // Base physical address of code segment
        LSR     r1, #20                    // Shift right to align to 1MB boundaries
        ORR     r3, r2, r1, LSL#20         // Setup the initial level1 descriptor again
        ORR     r3, r3, #0b0000000001100   // Set CB bits
        ORR     r3, r3, #0b1000000000000   // Set TEX bit 12
        STR     r3, [r0, r1, LSL#2]        // str table entry

//----------------------------------------------------------------
// Setup domain control register - Enable all domains to client mode
//----------------------------------------------------------------

        MRC     p15, 0, r0, c3, c0, 0      // Read Domain Access Control Register      
//        LDR     r0, =0xFFFFFFFF            // Initialize every domain entry to b11 (manager)
        LDR     r0, =0x55555555
        MCR     p15, 0, r0, c3, c0, 0      // Write Domain Access Control Register

//----------------------------------------------------------------
// Enable MMU and branch to main
// Leaving the caches disabled until after scatter loading.
//----------------------------------------------------------------

        MRC     p15, 0, r0, c1, c0, 0      // Read System Control Register
        BIC     r0, r0, #(0x1 << 12)       // Clear I bit 12 to disable I Cache
        BIC     r0, r0, #(0x1 <<  2)       // Clear C bit  2 to disable D Cache
        BIC     r0, r0, #0x2               // Clear A bit  1 to disable strict alignment fault checking
        ORR     r0, r0, #0x1               // Set M bit 0 to enable MMU before scatter loading
        MCR     p15, 0, r0, c1, c0, 0      // Write System Control Register
        ISB

// Now the MMU is enabled, virtual to physical address translations will occur. This will affect the next
// instruction fetch.
//
// The two instructions currently in the pipeline will have been fetched before the MMU was enabled.
// The branch to _start is safe because the Virtual Address (VA) is the same as the Physical Address (PA)
// (flat mapping) of this code that enables the MMU and performs the branch

	bl enable_caches
        bl main

//----------------------------------------------------------------
// Enable caches and branch prediction
// This code must be run from a privileged mode
//----------------------------------------------------------------

.align 3
.global enable_caches
.type enable_caches, "function"
enable_caches:

//----------------------------------------------------------------
// Enable caches and branch prediction
//----------------------------------------------------------------

        MRC     p15, 0, r0, c1, c0, 0      // Read System Control Register
        ORR     r0, r0, #(0x1 << 12)       // Set I bit 12 to enable I Cache
        ORR     r0, r0, #(0x1 << 2)        // Set C bit  2 to enable D Cache
        ORR     r0, r0, #(0x1 << 11)       // Set Z bit 11 to enable branch prediction
        MCR     p15, 0, r0, c1, c0, 0      // Write System Control Register
        ISB


//----------------------------------------------------------------
// Enable L1 D-side prefetch (A9 specific)
//----------------------------------------------------------------

        MRC     p15, 0, r0, c1, c0, 1      // Read Auxiliary Control Register
        ORR     r0, r0, #(0x1 << 2)        // Set DP bit 2 to enable L1 Dside prefetch
        MCR     p15, 0, r0, c1, c0, 1      // Write Auxiliary Control Register
        ISB

        BX      lr
