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

void uart_print(const char *string);
void uart_init();
EFI_STATUS memdump_to_file(const uint8_t *data, uint32_t size);
EFI_STATUS loadPayloadIntoMemory(EFI_PHYSICAL_ADDRESS memoryAddress, short unsigned int fileName[], size_t* fileSize);

void memcpy_usr(void* dest, const void* src, size_t n) {
	//cast src and dest to char*
	char *src_char = (char *)src;
	char *dest_char = (char *)dest;
	for (int i=0; i<n; i++) {
		dest_char[i] = src_char[i]; //copy contents byte by byte
	}
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

	//
	// Uncomment the following comment to dump memory!
	//

	/*
	uint8_t* pMem = (uint8_t*)0x80000000; // start of trustzone 
	//uint8_t* pMem = (uint8_t*)0xFB49A000; // start of high memory, also used by UEFI
	uint32_t size = 0x03000000; // covers the whole trustzone + some other memory
	//uint32_t size = 0x04A45FFF; // Address 0xFFEDFFFF; end of high memory

	// Dump memory to file

	Print(L"Start dumping to file!\n");
	uart_print("Start dumping to file!\r\n");

	Status = memdump_to_file(pMem, size);
	
	if (Status != EFI_SUCCESS)
	{
		Print(L"Failed at dumping to file!\n");
		uart_print("Failed at dumping to file!\r\n");
		CpuDeadLoop();
	}

	Print(L"Done dumping to file!\n");
	uart_print("Done dumping to file!\r\n");
	*/

	//
	// Put payload into memory
	//
	Print(L"Putting payload into memory!\n");
	uart_print("Putting payload into memory!\r\n");

	size_t fileSize1 = 0;
	Status = loadPayloadIntoMemory((EFI_PHYSICAL_ADDRESS)0x83000000, L"\\payload.bin", &fileSize1);
	
	if (Status != EFI_SUCCESS)
	{
		Print(L"Failed at loading payload!\n");
		uart_print("Failed at loading payload!\r\n");
		CpuDeadLoop();
	}

	Print(L"Payload is now in memory!\n");
	uart_print("Payload is now in memory!\r\n");

	char printBuffer[10];
	uart_print("Printing the 1st 4 bytes at 0x83000000!\r\n");
	uart_print(utoa(*((uint32_t*)(0x83000000)), printBuffer, 16));
	uart_print("\r\n");

	//
	// Put uboot into memory
	//
	Print(L"Putting uboot into memory!\n");
	uart_print("Putting uboot into memory!\r\n");

	size_t fileSize2 = 0;
	Status = loadPayloadIntoMemory((EFI_PHYSICAL_ADDRESS)0x84000000, L"\\uboot.bin", &fileSize2);
	
	if (Status != EFI_SUCCESS)
	{
		Print(L"Failed at loading uboot!\n");
		uart_print("Failed at loading uboot!\r\n");
		CpuDeadLoop();
	}

	Print(L"Uboot is now in memory!\n");
	uart_print("Uboot is now in memory!\r\n");

	uart_print("Printing the 1st 4 bytes at 0x84000000!\r\n");
	uart_print(utoa(*((uint32_t*)(0x84000000)), printBuffer, 16));
	uart_print("\r\n");

	Print(L"UEFI part finished. Setting up for SMC...\n");

	// Disable MMU to get access to Trustzone memory
	ArmDisableCachesAndMmu();

	// Copy payload into Trustzone memory. 
	// 0x8011219c is in the SMC handler, right after the NS bit was set to 0 in the SCR
	// The memory needs to be marked as secure, as you can only execute secure memory in secure mode
	memcpy_usr((void*)(0x8011219c), (const void*)0x83000000, (size_t)fileSize1);
	
	// Payload is now in place. Enable MMU for memory dump.
	ArmEnableMmu();
	ArmEnableDataCache();
	ArmEnableInstructionCache();
	
	uart_print("I just wrote Trustzone and I'm still alive!\r\n");

	// This should trigger an SMC, jump to the payload and output stuff to uart. Hopefully.
	ArmCallSmcHelper(0x03, 0x09, 0, 0);

	// We shouldn't get here since going back from the SMC in the payload isn't implemented
	Print(L"Something went wrong, we shouldn't be here\n");
	uart_print("Something went wrong, we shouldn't be here\r\n");

	CpuDeadLoop();
	return Status;
}

VOID FinalizeApp(VOID)
{
	// Let people wait for stroke
	Print(L"!!! PLEASE RESET YOUR DEVICE MANUALLY USING THE POWER BUTTON !!!\n");
	Print(L"!!! PLEASE RESET YOUR DEVICE MANUALLY USING THE POWER BUTTON !!!\n");
	Print(L"!!! PLEASE RESET YOUR DEVICE MANUALLY USING THE POWER BUTTON !!!\n");
	CpuDeadLoop();
}

EFI_STATUS loadPayloadIntoMemory(EFI_PHYSICAL_ADDRESS memoryAddress, short unsigned int fileName[], size_t* fileSize)
{
	EFI_LOADED_IMAGE *loaded_image = NULL;
    EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
    EFI_STATUS status;
	EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = NULL;

	status = gBS->HandleProtocol(
		gImageHandle, 
		&loaded_image_protocol, 
		(void **) &loaded_image);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at 1st HandleProtocol\n");
		return status;
	}

	status = gBS->HandleProtocol(
		loaded_image->DeviceHandle,
		&sfspGuid,
		(void**)&fs);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at 2nd HandleProtocol\n");
		return status;
	}

	EFI_FILE_PROTOCOL* fileProtocol = NULL;

	status = fs->OpenVolume(fs, &fileProtocol);
	EFI_FILE_PROTOCOL* file = NULL;

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at OpenVolume!\n");
		return status;
	}

	status = fileProtocol->Open(
			fileProtocol, 
			&file,
			fileName,
			EFI_FILE_MODE_READ,
			EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at opening file!\n");
		return status;
	}

	EFI_FILE_INFO *payloadFileInformation = NULL;
	UINTN payloadFileInformationSize = 0;

	status = file->GetInfo(
			file,
			&gEfiFileInfoGuid,
			&payloadFileInformationSize,
			(VOID *) payloadFileInformation);

	if (status != EFI_SUCCESS && status != EFI_BUFFER_TOO_SMALL)
	{
		Print(L"Failed at getting file info! (1)\n");
		return status;
	}

	status = gBS->AllocatePool(
		EfiLoaderData, 
		payloadFileInformationSize, 
		(void**)&payloadFileInformation);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at allocating pool!\n");
		return status;
	}

	SetMem(
		(VOID *)payloadFileInformation, 
		payloadFileInformationSize, 
		0xFF);

	status = file->GetInfo(
		file,
		&gEfiFileInfoGuid,
		&payloadFileInformationSize,
		(VOID *)payloadFileInformation
	);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at getting file info! (2)\n");
		return status;
	}

	EFI_PHYSICAL_ADDRESS payloadFileBuffer = memoryAddress;
	UINTN payloadFileSize = payloadFileInformation->FileSize;

	Print(L"Payloadsize: %u\n", payloadFileSize);

	UINTN Pages = EFI_SIZE_TO_PAGES (payloadFileSize);
	status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, Pages, &payloadFileBuffer);

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

	SetMem(
		(EFI_PHYSICAL_ADDRESS*)payloadFileBuffer,
		payloadFileSize,
		0xFF);

	status = file->Read(
		file,
		&payloadFileSize,
		(EFI_PHYSICAL_ADDRESS*)payloadFileBuffer
	);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at reading file into memory!\n");
		return status;
	}

	*fileSize = payloadFileSize;

	Print(L"File is now in memory at location 0x%x!\n", payloadFileBuffer);

	return EFI_SUCCESS;
}

/*
	Dump memory to a file on the EFI partition this program booted from
	@param data	Base memory address where to start dumping from
	@param size How man bytes to be dumped starting from the base memory address
*/
EFI_STATUS memdump_to_file(const uint8_t *data, uint32_t size) {

    EFI_STATUS status = EFI_SUCCESS;
	EFI_FILE_PROTOCOL* file = NULL;
	EFI_LOADED_IMAGE *loaded_image = NULL;
    EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
	EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* volumeHandle = NULL;
	EFI_FILE_PROTOCOL* rootDirectory = NULL;

	// Setup a buffer where we can copy trustzone memory to.
	void* memoryDumpBuffer = NULL;	

	status = gBS->AllocatePool(
		EfiLoaderData, 
		size, 
		&memoryDumpBuffer);

	if (status != EFI_SUCCESS)
	{
		if (status == EFI_INVALID_PARAMETER)
			Print(L"Invalid Paramter!\n");
		if (status == EFI_OUT_OF_RESOURCES)
			Print(L"Out of Resources!\n");
		if (status == EFI_NOT_FOUND)
			Print(L"Not Found!\n");
		
		Print(L"Failed at allocating payload buffer!\n");
		return EFI_SUCCESS;
	}

	Print(L"anything\n");

	// Get the loaded image handle
	status = gBS->HandleProtocol(
		gImageHandle, 
		&loaded_image_protocol, 
		(void **) &loaded_image);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at 1st HandleProtocol\n");
		uart_print("Failed at 1st HandleProtocol\r\n");
		if (status == EFI_INVALID_PARAMETER)
			uart_print("Invalid Paramter!\r\n");
		if (status == EFI_UNSUPPORTED)
			uart_print("Unsupported!\r\n");
		return status;
	}

	// Get the volume handle from the loaded image
	// This volume will always be the one where our EFI file is located
	status = gBS->HandleProtocol(
		loaded_image->DeviceHandle,
		&sfspGuid,
		(void**)&volumeHandle);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at 2nd HandleProtocol\n");
		uart_print("Failed at 2nd HandleProtocol\r\n");
		return status;
	}

	// Open the volume. We get a EFI_FILE_PROTOCOL (rootDirectory) to access the filesystem
	status = volumeHandle->OpenVolume(volumeHandle, &rootDirectory);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at OpenVolume!\n");
		uart_print("Failed at OpenVolume!\r\n");
		return status;
	}

	uart_print("Opening the \"memdump.bin\" file! (or creating it if it doesn't exist)\r\n");

	// Open the file \memdump.bin on the open filesystem. 
	// EFI_FILE_MODE_CREATE is supplied as mode, so the file will be created if it doesn't exist.
	// It will be created with no filesystem flags, like a hidden flag, as none are specified. (Replace (UNIT64)0)

	status = rootDirectory->Open(
		rootDirectory, 
		&file,
		L"\\memdump.bin",
		EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
		(UINT64)0);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at opening file!\n");
		uart_print("Failed at opening file!\r\n");
		return status;
	}

	uart_print("Opened file\r\n");

	// Our desired file is now open. 
	// At this point writing to it is possible, 
	// but that data still needs to be copied to an EFI buffer, 
	// as dumping from certain memory isn't possible

	ArmDataSynchronizationBarrier();
	ArmDisableCachesAndMmu();

	memcpy_usr(memoryDumpBuffer, data, size);

	ArmEnableMmu();
	ArmEnableDataCache();
	ArmEnableInstructionCache();	

	uart_print("Writing memory!\r\n");

	// Write the specified data. Also do some advanced error detection as it can be helpful.
	status = rootDirectory->Write(
		file,
		&size,
		memoryDumpBuffer);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at writing file!\n");
		uart_print("Failed at writing file!\r\n");
		if (status == EFI_WRITE_PROTECTED)
			uart_print("Write Protected!\r\n");
		if (status == EFI_UNSUPPORTED)
			uart_print("Writing is unsupported!\r\n");
		return status;
	}

	uart_print("Write command succeeded. Closing the file and flushing the cache!\r\n");

	// The file looks written, but data could still be cached. Close it so all operations stop and cache data will be written.
	status = rootDirectory->Close(file);
	
	uart_print("Closed file. USB can now be unplugged.\r\n");

	// The file is closed and everything is cleaned up. We can return.
	return EFI_SUCCESS;
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