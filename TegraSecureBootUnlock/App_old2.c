#include "Include/Application.h"


#define HEX_CHAR(x) 						((((x) + '0') > '9') ? ((x) + '7') : ((x) + '0'))

#define _REG(base, off) 					*(volatile unsigned int *)((base) + (off))
#define reg_write(base, off, value) 		_REG(base, off) = value
#define reg_clear(base, off, value) 		_REG(base, off) &= ~value
#define reg_set(base, off, value) 			_REG(base, off) |= value
#define reg_read(base, off) 				_REG(base, off)

#define PMC_BASE							(0x7000e400)
#define APBDEV_PMC_SCRATCH42_0				(0x144)
#define MAGIC_VALUE							(0x42)
#define APBDEV_PMC_IO_DPD_REQ_0				(0x1b8)
#define UART_DPD_BIT						(1 << 14)

#define PINMUX_BASE							(0x70003000)
/* uart-a tx */
#define PINMUX_AUX_ULPI_DATA0_0				(0x00)
/* uart-a rx */
#define PINMUX_AUX_ULPI_DATA1_0				(0x04)

#define CAR_BASE							(0x60006000)
#define CLK_SOURCE_PLLP						(0x0)

#define UART_THR_DLAB						(0x00)
#define UART_IER_DLAB						(0x04)
#define UART_IIR_FCR						(0x08)
#define UART_LCR							(0x0c)
#define UART_LSR							(0x14)

// 229 is guessed and work if booted from UEFI, APX needs 129
#define UART_RATE_115200					(229)
/* clear TX FIFO */
#define FCR_TX_CLR							(1 << 2)
/* clear RX FIFO */
#define FCR_RX_CLR							(1 << 1)
/* enable TX & RX FIFO */
#define FCR_EN_FIFO							(1 << 0)
/* Divisor Latch Access Bit */
#define LCR_DLAB							(1 << 7)
/* word length of 8 */
#define LCR_WD_SIZE_8						(0x3)

// uart a
#define UART_BASE 							(0x70006000)
#define CLK_RST_CONTROLLER_CLK_OUT_ENB 		(0x10)
#define CLK_RST_CONTROLLER_RST_DEVICES 		(0x04)
#define CLK_RST_CONTROLLER_CLK_SOURCE_UART	(0x178)
#define UART_CAR_MASK 						(1 << 6)

EFI_STATUS memdump_to_file(const uint8_t *data, uint32_t size);
void uart_print(const char *string);
void uart_init();
void uart_write(uint8_t data);
void uart_hexdump(const uint8_t *data, uint32_t size);
void uart_dump(const uint8_t *data, uint32_t size);
void prntnum(uint8_t n, int base, char sign, char *outbuf);

typedef void (*entry_point)(void);

uint32_t PointerToInt(void* ptr){
    uint32_t* u=(void*)&ptr;
    return *u;
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

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
char* itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

void memcpy_usr(void* dest, const void* src, size_t n) {
	/*char printBuffer[10];
	uart_print("Memcpy called \r\n");
	uart_print(utoa((unsigned int)src, printBuffer, 16));
	uart_print("\r\n");
	uart_print(utoa((unsigned int)dest, printBuffer, 16));
	uart_print("\r\n");
	uart_print(utoa((unsigned int)n, printBuffer, 16));
	uart_print("\r\n");*/
	//cast src and dest to char*
	char *src_char = (char *)src;
	char *dest_char = (char *)dest;
	for (int i=0; i<n; i++) {
		dest_char[i] = src_char[i]; //copy contents byte by byte
	}
}

EFI_STATUS tryStuff();

EFI_STATUS TegraSecureBootUnlockEntryPoint(
    EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS Status = EFI_SUCCESS;
	

	// Turn off watchdog timer, since this does take a while
	gBS->SetWatchdogTimer(0, 0, 0, NULL);

	// Search the proper entry
	// Status = LaunchExploitByVersionTable();
	Tegra3ConsoleOutputFixup();

	Print(L"Hello World\n");
	uart_print("Hello World.\r\n");


	uart_print("perform exploit\r\n");
	PerformNvTegra3Exploit();
	uart_print("done exploit\r\n");



	Print(L"Trying stuff!\n");
	uart_print("Trying stuff!\r\n");

	Status = tryStuff();

	if (Status != EFI_SUCCESS)
	{
		Print(L"Failed at dumping to file!\n");
		uart_print("Failed at dumping to file!\r\n");
		CpuDeadLoop();
	}

	Print(L"Tried stuff!\n");
	uart_print("Tried stuff!\r\n");



	/*uint8_t* pMem = (uint8_t*)0x80000C00;
	uint32_t size = 0x00000100;*/

	// Dump memory to file

	Print(L"Start dumping to file!\n");
	uart_print("Start dumping to file!\r\n");

	//Status = memdump_to_file(pMem, size);
	
	if (Status != EFI_SUCCESS)
	{
		Print(L"Failed at dumping to file!\n");
		uart_print("Failed at dumping to file!\r\n");
		CpuDeadLoop();
	}

	Print(L"Done dumping to file!\n");
	uart_print("Done dumping to file!\r\n");

	CpuDeadLoop();
	return Status;
}

EFI_STATUS tryStuff() {

	ArmDisableCachesAndMmu();

	char printBuffer[3];
	char temp1Buffer[3];
	char temp2Buffer[3];

	for (size_t i = 0; i < 0x100; i++)
	{
		utoa((unsigned int)(*((uint8_t*)(0x80000C00 + i))), temp1Buffer, 2);

		uart_print(printBuffer);
		uart_print("\r\n");
	}
	
	ArmEnableMmu();
	ArmEnableDataCache();
	ArmEnableInstructionCache();

	return EFI_SUCCESS;
}

struct MemCpyParameters {
	uint8_t* source, *dest;
	size_t size;
};

/*
	Dump memory to a file on the EFI partition this program booted from
	@param data	Base memory address where to start dumping from
	@param size How man bytes to be dumped starting from the base memory address
*/
EFI_STATUS memdump_to_file(const uint8_t *data, uint32_t size) {

	EFI_STATUS status = EFI_SUCCESS;

	char printBuffer[10];
	uart_print("1st debug point \r\n");
	uart_print(utoa((unsigned int)data, printBuffer, 16));
	uart_print("\r\n");
	uart_print(utoa(size, printBuffer, 16));
	uart_print("\r\n");
	
	ArmDisableCachesAndMmu();

	EFI_PHYSICAL_ADDRESS PhysicalBuffer = 0x83000000;
	UINTN Pages = EFI_SIZE_TO_PAGES (SIZE_16KB);
	status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, Pages, &PhysicalBuffer);

	if (status != EFI_SUCCESS)
	{
		switch (status)
		{
		case EFI_OUT_OF_RESOURCES:
			uart_print("EFI_OUT_OF_RESOURCES\r\n");
			break;
		case EFI_INVALID_PARAMETER:
			uart_print("EFI_INVALID_PARAMETER\r\n");
			break;
		case EFI_NOT_FOUND:
			uart_print("EFI_NOT_FOUND\r\n");
			break;
		default:
			break;
		}

		Print(L"Failed at AllocatePages!\n");
		uart_print("Failed at AllocatePages!\r\n");
		return status;
	}

	ArmEnableMmu();
	ArmEnableDataCache();
	ArmEnableInstructionCache();	

	void* memoryDumpBuffer = (void*)(UINTN)PhysicalBuffer;

	uart_print("Printing address of memoryDumpBuffer!\r\n");
	uart_print(utoa((unsigned int)(memoryDumpBuffer), printBuffer, 16));
	uart_print("\r\n");

	uart_print("Printing 0x83000000!\r\n");
	uart_print(utoa(0x83000000, printBuffer, 16));
	uart_print("\r\n");

	uart_print("2nd debug point               *(&pointer) \r\n");
	uart_print(utoa((unsigned int)*(&data), printBuffer, 16));
	uart_print("\r\n");
	uart_print(utoa(*(&size), printBuffer, 16));
	uart_print("\r\n");

	struct MemCpyParameters* MMU_enabled_addresses = (struct MemCpyParameters*)0x83000000;
	MMU_enabled_addresses->dest = (uint8_t*)memoryDumpBuffer;
	MMU_enabled_addresses->source = (uint8_t*)data;
	MMU_enabled_addresses->size = size;

	uart_print("3rd debug point \r\n");
	uart_print(utoa((unsigned int)(MMU_enabled_addresses->dest), printBuffer, 16));
	uart_print("\r\n");
	uart_print(utoa((unsigned int)(MMU_enabled_addresses->source), printBuffer, 16));
	uart_print("\r\n");
	uart_print(utoa((unsigned int)(MMU_enabled_addresses->size), printBuffer, 16));
	uart_print("\r\n");

	UINT32* thisIsAVariable = (UINT32 *)(EFI_PHYSICAL_ADDRESS)0x83000100;
	*thisIsAVariable = 25936;

	ArmDataSynchronizationBarrier();

	uart_print("Printing thisIsAVariable!\r\n");
	uart_print(utoa((*((UINT32 *)(EFI_PHYSICAL_ADDRESS)0x83000100)), printBuffer, 10));
	uart_print("\r\n");

	ArmDisableCachesAndMmu();

	char printBuffer2[10];

	uart_print("Printing thisIsAVariable!\r\n");
	uart_print(utoa((*((UINT32 *)(EFI_PHYSICAL_ADDRESS)0x83000100)), printBuffer2, 10));
	uart_print("\r\n");

	UINT32* thatIsAVariable = (UINT32 *)(EFI_PHYSICAL_ADDRESS)0x83000100;
	*thatIsAVariable = 25936*2;

	uart_print("Printing thatIsAVariable!\r\n");
	uart_print(utoa((*((UINT32 *)(EFI_PHYSICAL_ADDRESS)0x83000100)), printBuffer2, 10));
	uart_print("\r\n");

	/*void* dest = memoryDumpBuffer;
	const void* src = data;
	size_t l_size = size;*/

	struct MemCpyParameters* MMU_disabled_addresses = (struct MemCpyParameters*)0x83000000;
	uint8_t* dest = MMU_disabled_addresses->dest;
	uint8_t* src = MMU_disabled_addresses->source;
	size_t l_size = MMU_disabled_addresses->size;

	uart_print("Reading from address 0x00000000!\r\n");
	uart_print(utoa(*((char*)0x00000000), printBuffer2, 16));
	uart_print("\r\n");

	uart_print("4th debug point                *(&pointer) \r\n");
	uart_print(utoa((unsigned int)*(&data), printBuffer2, 16));
	uart_print("\r\n"); 
	uart_print(utoa((unsigned int)*(&size), printBuffer2, 16));
	uart_print("\r\n");

	uart_print("5th debug point \r\n");
	uart_print(utoa((unsigned int)dest, printBuffer2, 16));
	uart_print("\r\n");
	uart_print(utoa((unsigned int)src, printBuffer2, 16));
	uart_print("\r\n");
	uart_print(utoa((unsigned int)l_size, printBuffer2, 16));
	uart_print("\r\n");

//	memcpy_usr(memoryDumpBuffer, (const void*)data, (size_t)size);
	memcpy_usr(dest, src, l_size);

	ArmEnableMmu();
	ArmEnableDataCache();
	ArmEnableInstructionCache();	

	uart_print("Printing thatIsAVariable!\r\n");
	uart_print(utoa((*((UINT32 *)(EFI_PHYSICAL_ADDRESS)0x83000100)), printBuffer2, 10));
	uart_print("\r\n");

	ArmDisableCachesAndMmu();

	uart_print("Printing thisIsAVariable!\r\n");
	uart_print(utoa((*((UINT32 *)(EFI_PHYSICAL_ADDRESS)0x83000100)), printBuffer2, 10));
	uart_print("\r\n");

	uart_print("Printing thatIsAVariable!\r\n");
	uart_print(utoa((*((UINT32 *)(EFI_PHYSICAL_ADDRESS)0x83000100)), printBuffer2, 10));
	uart_print("\r\n");

	ArmEnableMmu();
	ArmEnableDataCache();
	ArmEnableInstructionCache();	

	uart_print("Printing thisIsAVariable!\r\n");
	uart_print(utoa((*((UINT32 *)(EFI_PHYSICAL_ADDRESS)0x83000100)), printBuffer, 10));
	uart_print("\r\n");

	uart_print("Printing thatIsAVariable!\r\n");
	uart_print(utoa((*((UINT32 *)(EFI_PHYSICAL_ADDRESS)0x83000100)), printBuffer, 10));
	uart_print("\r\n");

	uart_print("6th debug point \r\n");
	uart_print(utoa((unsigned int)data, printBuffer, 16));
	uart_print("\r\n");
	uart_print(utoa((unsigned int)size, printBuffer, 16));
	uart_print("\r\n");

	
	return EFI_SUCCESS;
}

void uart_write(const uint8_t data) {
	/* use this to keep track of if uart has been initialized */
	uart_init();

	/* put the char into the tx fifo */
	reg_write(UART_BASE, UART_THR_DLAB, data);

	/* wait for tx fifo to clear */
	while(!((reg_read(UART_BASE, UART_LSR) >> 5) & 0x01));

}

void uart_print(const char *string) {
	/* use this to keep track of if uart has been initialized */
	uart_init();
	/* send all characters until NULL to uart-N */
	while(*string) {
		/* put the char into the tx fifo */
		reg_write(UART_BASE, UART_THR_DLAB, (char) *string);
		/* wait for tx fifo to clear */
		while(!((reg_read(UART_BASE, UART_LSR) >> 5) & 0x01));
		/* move on to next char */
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