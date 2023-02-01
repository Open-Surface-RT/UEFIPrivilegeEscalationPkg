.globl main
main:
	isb
	dsb
	
	ldr r0, =0x88008000
	ldr r2, =0x00000000
loop:
	str r2, [r0]
	add r2, r2, #1
	dmb
	b loop

	
