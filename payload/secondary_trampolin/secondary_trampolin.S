.globl main
main:
	isb
	dsb
	
	ldr r0, =0x88008000
	ldr r1, =0x11223344
	ldr r2, =0xFFFFFFFF
loop:
	# str r1, [r0]
	# dmb
	str r2, [r0]
	sub r2, r2, #1
	dmb
	#smc #0
	b loop
