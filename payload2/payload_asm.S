.text

// 48 bytes is a bit overkill, but it makes sure we don't override any stack
// make sure to change 'void clean_buffer(void* buffer)' in payload.c if you change this.
utoabuffer: .word 48

.global register_print
.type register_print, function
register_print:
	// Push return address onto stack
	push {lr}

	// Read CPSR into r0 and print the register
	mrs r0, CPSR
	bl print_r0

// 8-byte align the read of the SCR. This makes everything work. I don't know why
.align 5
	// Read NS bit into r0 and print the register
	mrc p15, 0, r0, c1, c1, 0
	bl print_r0

	// Read back return address and return to the C code
	pop {pc}

// Print the r0 register to serial and use utoa to format it as binary
print_r0:
	// Push the used registers, just to be safe
	push {lr}

	// Setup for utoa call. r0 was setup before we got into this subroutine
	adr r1, utoabuffer
	mov r2, #2

	// r0 is input, r1 is destination buffer, r2 is base
	bl utoa

	// Execute uart_print. r0 is the string's address
	adr r0, utoabuffer
	bl uart_print

	// Print a new line
	bl print_newline

	// Restore registers and go back
	pop {pc}

.global dead_loop
.type dead_loop, function
dead_loop:
	b dead_loop