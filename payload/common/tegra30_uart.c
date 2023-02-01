#include "tegra30_uart.h"
#include <stdarg.h>


// Make it easy to print a new line from assembly
void print_newline() {
	uart_print("\r\n");
}

// Debug uart print. Use if you want to know the memory address of the printed buffer.
void debug_uart_print(const char* string) {
	char printBuffer[16];
	uart_print("Address of string: ");
	uart_print(utoa((unsigned int)string, printBuffer, 16));
	uart_print("\r\n");
	uart_print(string);
}

char* utoa(unsigned int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

char* utoa_leading_zero(unsigned int value, char* result, int base, int stringsize) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value, counter = 0;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
		counter++;
    } while ( value );

	while (counter < stringsize) {
		*ptr++ = '0';
		counter++;
	}

    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

void putc(int c, void *stream) {
	// use this to keep track of if uart has been initialized
	uart_init();

	// put the char into the tx fifo
	reg_write(UART_BASE, UART_THR_DLAB, c);

	// wait for tx fifo to clear
	while(!((reg_read(UART_BASE, UART_LSR) >> 5) & 0x01));

}

void uart_print(const char *string) {
	// use this to keep track of if uart has been initialized
	uart_init();
	
	
	// Mutex stuff
	while (reg_read(PMC_BASE, APBDEV_PMC_SCRATCH41_0)) {
		; // wait until mutex is released
	}
	
	// take uart_mutex
	reg_write(PMC_BASE, APBDEV_PMC_SCRATCH41_0, 1);
	
	
	/*
	char buffer[100];
	char *string = buffer;
	
	va_list valst;
	
	va_start(valst, fmt);
	sprintf(buffer, fmt, valst);
	va_end(valst);
*/
	// send all characters until NULL to uart-N
	while(*string) {
		// put the char into the tx fifo
		reg_write(UART_BASE, UART_THR_DLAB, (char) *string);

		// wait for tx fifo to clear
		while(!((reg_read(UART_BASE, UART_LSR) >> 5) & 0x01));

		// move on to next char
		++string;
	}
	
	// release uart_mutex
	reg_write(PMC_BASE, APBDEV_PMC_SCRATCH41_0, 0);
}


void uart_init() {
	if(reg_read(PMC_BASE, APBDEV_PMC_SCRATCH42_0) != MAGIC_VALUE) {

		/* set pinmux for uart-a (surface RT) */
		reg_write(PINMUX_BASE, PINMUX_AUX_ULPI_DATA0_0, 0b00000110); /* tx */
		reg_write(PINMUX_BASE, PINMUX_AUX_ULPI_DATA1_0, 0b00100110); /* rx */


		/* clear deep power down for all uarts */
		reg_clear(PMC_BASE, APBDEV_PMC_IO_DPD_REQ_0, UART_DPD_BIT);

		/* enable clock for uart-N */
		reg_set(CAR_BASE, CLK_RST_CONTROLLER_CLK_OUT_ENB, UART_CAR_MASK);

		/* assert and deassert reset for uart-N */
		reg_set(CAR_BASE, CLK_RST_CONTROLLER_RST_DEVICES, UART_CAR_MASK);
		reg_clear(CAR_BASE, CLK_RST_CONTROLLER_RST_DEVICES, UART_CAR_MASK);

		/* set clock source to pllp for uart-N */
		reg_write(CAR_BASE, CLK_RST_CONTROLLER_CLK_SOURCE_UART, CLK_SOURCE_PLLP);

		/* set DLAB bit to enable programming of DLH/DLM registers */
		reg_set(UART_BASE, UART_LCR, LCR_DLAB);

		/* write lower 8 (DLH) and upper 8 (DLM) bits of 16 bit baud divisor */
		reg_write(UART_BASE, UART_THR_DLAB, (UART_RATE_115200 & 0xff));
		reg_write(UART_BASE, UART_IER_DLAB, (UART_RATE_115200 >> 8));

		/* clear DLAB bit to disable setting of baud divisor */
		reg_clear(UART_BASE, UART_LCR, LCR_DLAB);

		/* 8-bit word size - defaults are no parity and 1 stop bit */
		reg_write(UART_BASE, UART_LCR, LCR_WD_SIZE_8);

		/* enable tx/rx fifos */
		reg_write(UART_BASE, UART_IIR_FCR, FCR_EN_FIFO);

		/* prevent this uart-N initialization from being done on subsequent calls to uart_print() */
		reg_write(PMC_BASE, APBDEV_PMC_SCRATCH42_0, MAGIC_VALUE);
		
		/* use this reg as UART_MUTEX in a multicore set up :) */
		reg_write(PMC_BASE, APBDEV_PMC_SCRATCH41_0, 0);
	}
}

