#define UART_SHIFT 2

.globl main
main:
	isb
	dsb
	
	ldr r0, =0x88008000
	ldr r2, =0x00000000
	ldr r3, =0x70006000
	ldr r5, =200000
	ldrb r4, ='A'
loop:
	str r2, [r0]
	add r2, r2, #1
	cmp r2, r5
	bne loop
	
	strb	r4, [r3]
	add r4, r4, #1
	ldr r2, =0x00000000
	
	b loop
