﻿#include "Include/Application.h"


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
EFI_STATUS loadPayloadIntoMemory(EFI_PHYSICAL_ADDRESS memoryAddress, short unsigned int fileName[], size_t* fileSize);

typedef void (*entry_point)(void);

uint32_t PointerToInt(void* ptr){
    uint32_t* u=(void*)&ptr;
    return *u;
}

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

	register uint32_t r0 __asm__ ("r0");
	__asm__ ("mrs r0, CPSR" : : : "%r0");
	Print(L"ExceptionLevel: ");
	Print(L"0x%x", r0);
	Print(L"\n");

	char printBuffer1[3];
	printBuffer1[0] = "0123456789ABCDEF"[r0 / 10];
	printBuffer1[1] = "0123456789ABCDEF"[r0 % 10];
	printBuffer1[2] = '\0';

	uart_print("Exeption Level: 0x");
	uart_print(printBuffer1);
	uart_print("\r\n");

	uart_print("perform exploit\r\n");
	PerformNvTegra3Exploit();
	uart_print("done exploit\r\n");

	//
	// Uncomment the following comment to dump memory!
	//

	//uint8_t* pMem = (uint8_t*)0x80000000;
	//uint8_t* pMem = (uint8_t*)0xFB49A000;
	//uint32_t size = 0x03000000;
	//uint32_t size = 0x04A45FFF; // Address 0xFFEDFFFF

	// Dump memory to file

	/*Print(L"Start dumping to file!\n");
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

	goto end;*/


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

	// uncomment this to load a string into memory, which the payload can print
	//memcpy_usr((void*)0x85000000, "Very interesting message\r\n", 27);

	Print(L"Jumping not!\n");

	/*uart_print("Jumping to 0x83000000!\r\n");
	void (*foo)(void) = (void (*)())0x83000000;
	foo();*/

	uart_print("We are back to UEFI!\r\n");
	Print(L"Back!\n");

	//goto end;

	// Write SMC memory with jumps to 0x82000000
	// This can be done with disabling MMU,
	// then writing 
	// 	82 04 a0 e3	// mov r0, #0x82000000
	// 	10 ff 2f e1	// bx r0
	// these instructions to fec22840 continuously
	// until fec22970, there the last bx from the smc's happens.
	// Could be a bit overkill, who knows.
	// To see if overriding was successful, a memory dump could be made.

	// Printing something because that helped with memory dumping.
	Print(L"Printing something for good measure!\n");

	// Disable MMU to get access to Trustzone memory
	ArmDisableCachesAndMmu();

	// Create a buffer with the instructions
	char instructions[] = { 0x82, 0x04, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1 };
	//char instructions[] = { 0x00, 0xf0, 0x20, 0xe3, 0x00, 0xf0, 0x20, 0xe3 };

	// Now write the buffer to our desired memory location.
	/*for (size_t i = 0xfec22840; i < 0xfec22970; i+=8)
		for (size_t j = 0; j < 8; j++)
		*(((char*)i) + j) = instructions[j];*/

	// 0x80000c0{0|1} doesn't block execution
	// 0x8011243{0|1} doesn't block execution
	// 0x811f801{0|1|4} freezes, USB doesn't turn on light when plugged in again.
	// 0x812e1290 froze at writing TZ memory the 1st time, the second time didn't block execution, plugging in USB again made a blue line onscreen.
	// 0x819c9290 didn't test
	// 0x82000c0{0|4} trouble writing memory. (twice)
	// 0x811f8000 is monitor vector base address. Overwritting it makes a SMC jump to the payload.
	// 0x80112220 makes SMC jump to payload but doesn't make it execute in secure mode.

	for (size_t i = 0x8011219c; i < (0x8011219c + 0x00000100); i+=8)
		for (size_t j = 0; j < 8; j++)
			*(((char*)i) + j) = instructions[j];

	/*for (size_t i = 0x80000000; i < 0x82000000; i+=8)
		for (size_t j = 0; j < 8; j++)
			*(((char*)i) + j) = instructions[j];*/

	memcpy_usr((void*)(0x82000000), (const void*)0x83000000, (size_t)fileSize1);
	//memcpy_usr((void*)(0x81677680), (const void*)0x83000000, (size_t)fileSize1);
	//memcpy_usr((void*)(0x80108000), (const void*)0x84000000, (size_t)fileSize2);
	
	// Instructions are now in place. Enable MMU for memory dump.
	ArmEnableMmu();
	ArmEnableDataCache();
	ArmEnableInstructionCache();
	
	//Print(L"Printing something for good measure!\n");
	uart_print("I just wrote Trustzone and I'm still alive!\r\n");

	/*register uint32_t r0 __asm__ ("r0");
	r0 = 0x81000000;*/

	/*ArmDisableInterrupts();
	ArmDisableAsynchronousAbort();
	ArmDisableCachesAndMmu();**/


	// This should trigger an SMC, jump to the payload and output stuff to uart. Hopefully.
	ArmCallSmcHelper(0x03, 0x09, 0, 0);

	//goto dump;

	/*uart_print("Jumping to 0x78563412 payload!\r\n");
	void (*func_ptr2)(void) = (void (*)())0x78563412;;
	func_ptr2();

	uart_print("lets go!\r\n");
	void (*foo)(void) = (void (*)())0x80005004;
	foo();

	uart_print("I am back!\r\n");

	// Read r4
	register uint32_t r0 __asm__ ("r0");
	Print(L"r0: ");
	Print(L"0x%x", r0);
	Print(L"\n");

	uart_print("We are back to UEFI!\r\n");	*/

//end:

	Print(L"Finished!\n");
	uart_print("Finished!\r\n");

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
	//EFI_PHYSICAL_ADDRESS payloadFileBuffer = 0;
	UINTN payloadFileSize = payloadFileInformation->FileSize;

	Print(L"Payloadsize: %u\n", payloadFileSize);

	/*status = gBS->AllocatePool(
			EfiLoaderData, 
			payloadFileSize, 
			(void**)&payloadFileBuffer);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at allocating payload buffer!\n");
		return status;
	}*/

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

	/*ArmDisableCachesAndMmu();
	ArmInvalidateTlb();
	ArmInvalidateDataCache();
	ArmInvalidateInstructionCache();*/

	/*ArmDataMemoryBarrier();
	ArmDataSynchronizationBarrier();*/
	/*ArmCleanInvalidateDataCache();*/

	/*ArmEnableMmu();
	ArmEnableDataCache();
	ArmEnableInstructionCache();*/

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

	// Open the file \memdump.bin on the open filesystem. 
	// EFI_FILE_MODE_CREATE is supplied as mode, so the file will be created if it doesn't exist.
	// It will be created with no filesystem flags, like a hidden flag, as none are specified. (Replace (UNIT64)0)
	//uart_print("Opening the \"memdump.bin\" file! (or creating it if it doesn't exist)\r\n");
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

	// Our desired file is now open. At this point writing to it is possible, but that data still needs to be prepared.

	// Set the buffer to all-'z'
	/*SetMem(
		memoryDumpBuffer,
		size,
		0x7a);*/

	//const char* secretMessage = "This is a super-top-secrect message. No one should ever read it.";

	//uart_print("Printing some variables to the screen!\r\n");
	//Print(L"data: 0x%x\n", data);
	//Print(L"size: 0x%x\n", size);
	//Print(L"secrectMessage: 0x%x\n", secretMessage);
	//Print(L"memoryDumpBuffer: 0x%x\n", memoryDumpBuffer);

	/*for (size_t i = 0; i < 0x100; i++)
	{
		uart_print("This is a looooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooot of info\r\n");
	}*/
	

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

/*
	Write some text to file
*/
EFI_STATUS text_to_file() {

    EFI_STATUS status;
	EFI_FILE_PROTOCOL* file = NULL;
	EFI_LOADED_IMAGE *loaded_image = NULL;
    EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
	EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* volumeHandle = NULL;
	EFI_FILE_PROTOCOL* rootDirectory = NULL;

	// Get the loaded image handle
	status = gBS->HandleProtocol(
		gImageHandle, 
		&loaded_image_protocol, 
		(void **) &loaded_image);

	if (status != EFI_SUCCESS)
	{
		Print(L"Failed at 1st HandleProtocol\n");
		uart_print("Failed at 1st HandleProtocol\r\n");
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

	// Open the file \memdump.bin on the open filesystem. 
	// EFI_FILE_MODE_CREATE is supplied as mode, so the file will be created if it doesn't exist.
	// It will be created with no filesystem flags, like a hidden flag, as none are specified. (Replace (UNIT64)0)
	uart_print("Opening the \"memdump.bin\" file! (or creating it if it doesn't exist)\r\n");
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

	// Our desired file is now open. At this point writing to it is possible, but that data still needs to be prepared. 

	char bufferFileContent[] = "This is a very interesting text. It should be writting to a file, so we'll never loose this text.";
	UINT32 bufferFileSize = sizeof(bufferFileContent);

	// Write the specified data. Also do some advanced error detection as it can be helpful.
	status = rootDirectory->Write(
		file,
		&bufferFileSize,
		bufferFileContent);

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

	uart_print("Done writing file\r\n");

	// The file looks written, but data could still be cached. Close it so all operations stop and cache data will be written.
	status = rootDirectory->Close(file);
	
	uart_print("Closed file\r\n");

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

void uart_write(const uint8_t data) {
	/* use this to keep track of if uart has been initialized */
	uart_init();

	/* put the char into the tx fifo */
	reg_write(UART_BASE, UART_THR_DLAB, data);

	/* wait for tx fifo to clear */
	while(!((reg_read(UART_BASE, UART_LSR) >> 5) & 0x01));

}

void uart_hexdump(const uint8_t *data, uint32_t size) {
	static char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	char result[2];
	
	/* use this to keep track of if uart has been initialized */
	uart_init();

	while (size)
	{
		result[0] = hex[(*data >> 8) % 16];
		result[1] = hex[*data % 16];
		
		reg_write(UART_BASE, UART_THR_DLAB, (char) result[0]);
		while(!((reg_read(UART_BASE, UART_LSR) >> 5) & 0x01));

		reg_write(UART_BASE, UART_THR_DLAB, (char) result[1]);
		while(!((reg_read(UART_BASE, UART_LSR) >> 5) & 0x01));

		/* move on to next char */
		++data;
		--size;
	}
}

void uart_dump(const uint8_t *data, uint32_t size) {
	/* use this to keep track of if uart has been initialized */
	uart_init();

	/* send all characters until NULL to uart-N */
	while(size) {
		/* put the char into the tx fifo */
		reg_write(UART_BASE, UART_THR_DLAB, (char) *data);

		/* wait for tx fifo to clear */
		while(!((reg_read(UART_BASE, UART_LSR) >> 5) & 0x01));

		/* move on to next char */
		++data;
		--size;
	}
}

void prntnum(uint8_t num, int base, char sign, char *outbuf)
{
	outbuf[0] = "0123456789ABCDEF"[num / base];
	outbuf[1] = "0123456789ABCDEF"[num % base];
	outbuf[2] = '\0';
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