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


#define _REG(base, off) 				*(volatile unsigned int *)((base) + (off))
#define reg_write(base, off, value) 			_REG(base, off) = value
#define reg_clear(base, off, value) 			_REG(base, off) &= ~value
#define reg_set(base, off, value) 			_REG(base, off) |= value
#define reg_read(base, off) 				_REG(base, off)

#define HEX_CHAR(x) ((((x) + '0') > '9') ? ((x) + '7') : ((x) + '0'))

#define PMC_BASE					(0x7000e400)
#define APBDEV_PMC_SCRATCH42_0			(0x144)
#define MAGIC_VALUE					(0x42)
#define APBDEV_PMC_IO_DPD_REQ_0			(0x1b8)
#define UART_DPD_BIT					(1 << 14)

#define PINMUX_BASE					(0x70003000)
/* uart-a tx */
#define PINMUX_AUX_ULPI_DATA0_0			(0x00)
/* uart-a rx */
#define PINMUX_AUX_ULPI_DATA1_0			(0x04)

#define CAR_BASE					(0x60006000)
#define CLK_SOURCE_PLLP				(0x0)

#define UART_THR_DLAB					(0x00)
#define UART_IER_DLAB					(0x04)
#define UART_IIR_FCR					(0x08)
#define UART_LCR					(0x0c)
#define UART_LSR					(0x14)

/* aka 408000000/115200/29 - works for IROM, assuming 408000000 PLLP */
// 122 is original for APX
#define UART_RATE_115200				(229)
/* clear TX FIFO */
#define FCR_TX_CLR					(1 << 2)
/* clear RX FIFO */
#define FCR_RX_CLR					(1 << 1)
/* enable TX & RX FIFO */
#define FCR_EN_FIFO					(1 << 0)
/* Divisor Latch Access Bit */
#define LCR_DLAB					(1 << 7)
/* word length of 8 */
#define LCR_WD_SIZE_8					(0x3)

// uart a
#define UARTA_BASE					(0x70006000)
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_L_0 		(0x10)
#define CLK_RST_CONTROLLER_RST_DEVICES_L_0 		(0x04)
#define CLK_RST_CONTROLLER_CLK_SOURCE_UARTA_0 	(0x178)
#define UARTA_CAR_MASK 				(1 << 6)

#define UART_BASE 					UARTA_BASE
#define CLK_RST_CONTROLLER_CLK_OUT_ENB 		CLK_RST_CONTROLLER_CLK_OUT_ENB_L_0
#define CLK_RST_CONTROLLER_RST_DEVICES 		CLK_RST_CONTROLLER_RST_DEVICES_L_0
#define CLK_RST_CONTROLLER_CLK_SOURCE_UART 		CLK_RST_CONTROLLER_CLK_SOURCE_UARTA_0
#define UART_CAR_MASK 					UARTA_CAR_MASK

void uart_print(const char *string);
void uart_init();
char* utoa(unsigned int value, char* result, int base);
char* utoa_leading_zero(unsigned int value, char* result, int base, int stringsize);
void memory_dump_uart(uint32_t* start, int size);
void modify_cps_reg();

// These are defined in payload_asm.S
extern void assembly_code();
extern void dead_loop();
extern void set_cpsr();

void main()
{
	uart_print("Welcome to the payload!\r\n");

	// Execute the assembly code, which prints some important registers 
	// and makes sure we are executing in secure mode.
	assembly_code();

	uart_print("Setting CPSR.M to SVC\r\n");

	set_cpsr();

	uart_print("Doing stuff with CSR Register!\r\n");
	// Changing cps reg with this doesn't work
	modify_cps_reg();

	uart_print("Memory-dumping over UART\r\n");
	// trying to dump this makes the screen go blue and doesn't print anyhting to uart
	memory_dump_uart((uint32_t*)0xFFF00000, 0x100);
	// trying to dump this works
	//memory_dump_uart((uint32_t*)0x6000C200, 0x100);

	uart_print("Finished executing payload. Going into a dead loop now!\r\n");

	// Dead loop. We don't want to execute random memory. 
	// Returning to UEFI Application doesn't work either. (at least right now)
	dead_loop();
}

void modify_cps_reg()
{
	char printBuffer[8];

	uint32_t* cps_reg = (uint32_t*)0x6000C200;

	uart_print("Printing CSR Register!\r\n");
	uart_print(utoa_leading_zero(*cps_reg, printBuffer, 16, 8));
	uart_print("\r\n");

	//*cps_reg = *cps_reg && 0b1111111111111110;
	//*cps_reg = *cps_reg || 0b0000000000100000;
	*cps_reg = 0b0000000000100001;
	//*cps_reg = 0x13;

	uart_print("Printing CSR Register!\r\n");
	uart_print(utoa_leading_zero(*cps_reg, printBuffer, 16, 8));
	uart_print("\r\n");
}

void memory_dump_uart(uint32_t* start, int size)
{
	char printBuffer[8];
	while (size > 0)
	{
		uart_print(utoa_leading_zero(*start, printBuffer, 16, 8));
		size -= 4;
		start++;
		if  (size % 16 == 0)
			uart_print("\r\n");
	}
	
}

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

void uart_print(const char *string) {
	// use this to keep track of if uart has been initialized
	uart_init();

	// send all characters until NULL to uart-N
	while(*string) {
		// put the char into the tx fifo
		reg_write(UART_BASE, UART_THR_DLAB, (char) *string);

		// wait for tx fifo to clear
		while(!((reg_read(UART_BASE, UART_LSR) >> 5) & 0x01));

		// move on to next char
		++string;
	}
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
	}
}