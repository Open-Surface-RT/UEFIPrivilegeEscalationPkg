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
	char printBuffer[16];
	uart_print("Memcpy called \r\n");
	uart_print(itoa((int)src, printBuffer, 16));
	uart_print("\r\n");
	uart_print(itoa((int)dest, printBuffer, 16));
	uart_print("\r\n");
	uart_print(itoa((int)dest, printBuffer, 16));
	uart_print("\r\n");
	//cast src and dest to char*
	char *src_char = (char *)src;
	char *dest_char = (char *)dest;
	for (int i=0; i<n; i++) {
		dest_char[i] = src_char[i]; //copy contents byte by byte
	}
}


EFI_STATUS TegraSecureBootUnlockEntryPoint(
    EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS Status = EFI_SUCCESS;
	/*EFI_LOADED_IMAGE *loaded_image = NULL;
  	EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
  	EFI_STATUS status;
	EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = NULL;
	EFI_FILE_PROTOCOL* fileProtocol = NULL;
	EFI_FILE_PROTOCOL* file = NULL;
	EFI_FILE_INFO *payloadFileInformation = NULL;
	UINTN payloadFileInformationSize = 0;*/
	/*UINTN MemMapSize = 0;
	EFI_MEMORY_DESCRIPTOR* MemMap = 0;
	UINTN MapKey = 0;
	UINTN DesSize = 0;
	UINT32 DesVersion = 0;*/

	// Turn off watchdog timer, since this does take a while
	gBS->SetWatchdogTimer(0, 0, 0, NULL);

	// Search the proper entry
	// Status = LaunchExploitByVersionTable();
	Tegra3ConsoleOutputFixup();

	Print(L"Hello World\n");
	uart_print("Hello World.\r\n");
	
	// Read EL
	/*register uint32_t r0 __asm__ ("r0");
	__asm__ ("mrs r0, CPSR" : : : "%r0");
	Print(L"ExceptionLevel: ");
	Print(L"0x%x", r0);
	Print(L"\n");*/
	
	/*
	status = gBS->HandleProtocol(
		ImageHandle, 
		&loaded_image_protocol, 
		(void **) &loaded_image);
	if (status != EFI_SUCCESS) {
		Print(L"Failed at 1st HandleProtocol\n");
		return EFI_SUCCESS;
	}


	status = gBS->HandleProtocol(
		loaded_image->DeviceHandle,
		&sfspGuid,
		(void**)&fs);
	if (status != EFI_SUCCESS) {
		Print(L"Failed at 2nd HandleProtocol\n");
		return EFI_SUCCESS;
	}


	// Open Filesystem
	status = fs->OpenVolume(fs, &fileProtocol);
	if (status != EFI_SUCCESS) {
		Print(L"Failed at OpenVolume!\n");
		return EFI_SUCCESS;
	}


	// Open uboot.bin
	status = fileProtocol->Open(
			fileProtocol, 
			&file,
			L"\\uboot.bin",
			EFI_FILE_MODE_READ,
			EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
	if (status != EFI_SUCCESS) {
		Print(L"Failed at opening file!\n");
		return EFI_SUCCESS;
	}


	// Get info of uboot.bin
	status = file->GetInfo(
			file,
			&gEfiFileInfoGuid,
			&payloadFileInformationSize,
			(VOID *) payloadFileInformation);
	if (status != EFI_SUCCESS && status != EFI_BUFFER_TOO_SMALL) {
		Print(L"Failed at getting file info! (1)\n");
		return EFI_SUCCESS;
	}
	

	// Allocate memory for uboot.bin
	status = gBS->AllocatePool(
		EfiLoaderData, 
		payloadFileInformationSize, 
		(void**)&payloadFileInformation);
	if (status != EFI_SUCCESS) {
		Print(L"Failed at allocating pool!\n");
		return EFI_SUCCESS;
	}
	

	// init payload memory with 0xFF
	SetMem(
		(VOID *)payloadFileInformation, 
		payloadFileInformationSize, 
		0xFF);


	// Getting File info again?
	status = file->GetInfo(
		file,
		&gEfiFileInfoGuid,
		&payloadFileInformationSize,
		(VOID *)payloadFileInformation
	);
	if (status != EFI_SUCCESS) {
		Print(L"Failed at getting file info! (2)\n");
		return EFI_SUCCESS;
	}

	void* payloadFileBuffer;
	UINTN payloadFileSize = payloadFileInformation->FileSize;

	Print(L"Payloadsize: %u\n", payloadFileSize);

	status = gBS->AllocatePool(
			EfiLoaderData, 
			payloadFileSize, 
			&payloadFileBuffer);
	if (status != EFI_SUCCESS) {
		Print(L"Failed at allocating payload buffer!\n");
		return EFI_SUCCESS;
	}
	

	SetMem(
		payloadFileBuffer,
		payloadFileSize,
		0xFF);


	// Read file to buffer
	status = file->Read(
		file,
		&payloadFileSize,
		payloadFileBuffer
	);
	if (status != EFI_SUCCESS) {
		Print(L"Failed at reading file into memory!\n");
		return EFI_SUCCESS;
	} else {
		Print(L"File is now in memory at location 0x%x!\n", payloadFileBuffer);
	}*/


	/*Print(L"MemoryMap:\n");
	gBS->GetMemoryMap(
		&MemMapSize, 
		MemMap, 
		&MapKey, 
		&DesSize, 
		&DesVersion
	);
	
	Print(L"MemMapSize: %d\n", MemMapSize);
	Print(L"MapKey: %d\n", MapKey);
	Print(L"DescriptorSize: %d\n", DesSize);
	Print(L"DescriptorVersion: %d\n", DesVersion);

	Print(L"End of MemoryMap\n");*/


/*
	Print(L"Exit BootService\r\n");
	uart_print("exit BS\r\n");
	gBS->GetMemoryMap(
		&MemMapSize, 
		MemMap, 
		&MapKey, 
		&DesSize, 
		&DesVersion
	);

	// ExitBootService - no UEFI functions from here
	Status = gBS->ExitBootServices(
		ImageHandle, 
		MapKey
	);
	if (EFI_ERROR(Status)) {
		uart_print("boot service didnt went brr...");
		Print(L"Failed to exit BS\n");
		return Status;
	}
*/
	
/*
	uart_print("Read back memory from UEFI location: \r\n");
	//uint32_t NOP = __builtin_bswap32(0x0000a0e1);
	//uint32_t* testPTR = (uint32_t*) payloadFileBuffer;
*/
	//*testPTR = NOP;
/*
	uint8_t* dataPTR = (uint8_t*) payloadFileBuffer;
	
	for (int i = 0; i < payloadFileSize; i++) {
		if (i % 16 == 0){
			uart_print("\r\n");
		} else if (i % 8 == 0) {
			uart_print(" ");
		}
*/
		//uint8_t data = dataPTR[i]; //*(dataPTR+i);
/*
		char s[3];
		prntnum(data, 16, ' ', s);
		uart_print(s);
	}
	uart_print("\r\nEND of memory read.\r\n");
	uart_print("\r\n\r\n");
*/

	/*void* dest = (void *) 0x83000000;
	void* src = payloadFileBuffer;
	size_t n = payloadFileSize;
	memcpy_usr(dest, src, n);*/

/*
	uart_print("Read back memory from COPY location: \r\n");
	uint32_t* dataPTR2 = (uint32_t*) dest;
	
	for (int i = 0; i*4 < payloadFileSize; i++) {
		if (i % 16 == 0){
			uart_print("\r\n");
		} else if (i % 8 == 0) {
			uart_print(" ");
		}

		uint32_t data = dataPTR2[i];
		char s[3];
		prntnum((uint8_t)data, 16, ' ', s);
		uart_print(s);
	}
	uart_print("\r\nEND of memory read.\r\n");

	uart_print("\r\n\r\n");
*/

/*
	uart_print("Start MEM compare");
	uint8_t* ptrUEFI = (uint8_t*) payloadFileBuffer;
	uint8_t* ptrCOPY = (uint8_t*) dest;
	char err[2] = "n\0";
	for (int i = 0; i < payloadFileSize; i++)
	{
		uint8_t dataUEFI = *(ptrUEFI+i);
		uint8_t dataCOPY = *(ptrCOPY+i);

		if (i % 16 == 0){
			uart_print("\r\n");
		} else if (i % 8 == 0) {
			uart_print(" ");
		}

		if (dataUEFI == dataCOPY) {
			uart_print("o");
		} else {
			uart_print("x");
			err[0] = 'y'; 
		}
	}
	uart_print("\r\nERR: ");
	uart_print(err);
	uart_print("\r\n\r\n");
*/	

	/*
	uart_print("Jumping to UEFI payload!\r\n");
	uint32_t address;
	address = PointerToInt(payloadFileBuffer);
	address = 0xFC3D6C90; //debug only
	void (*func_ptr)(void) = (void (*)())address;
	func_ptr();
	*/



	uart_print("perform exploit\r\n");
	PerformNvTegra3Exploit();
	uart_print("done exploit\r\n");
	/*ArmDisableCachesAndMmu();
	UINT32 Something = *((UINT32 *)(EFI_PHYSICAL_ADDRESS)0x80000000);*/
	

/*
	uint32_t NOP = __builtin_bswap32(0x0000a0e1); // convert big to little endian
	uint32_t* baseAddress = (uint32_t*) 0x80000000;
	for(int i = 0; i*4 < 0x02000000; i++) {
		baseAddress[i] = NOP;
	}
*/
	// dump TZ mempory
	/*uart_print("MMU down\r\n");
	uart_print("START_MEMDUMP");*/

	//uart_hexdump(pMem, size);

	//for (uint64_t i = 0; i < 0x02000000; i++) {
		/*
		if (i % 16 == 0){
			uart_print("\r\n");
		} else if (i % 8 == 0) {
			uart_print(" ");
		}
		*/

		//uint8_t data = *pMem++; //*(dataPTR+i);
		/*
		char s[3];
		prntnum(data, 16, ' ', s);
		uart_print(s);
		*/
		//uart_write(data);
//	}

	/*uart_print("END_MEMDUMP");
	uart_print("\r\n\r\n");
	uart_print("Done looking at TZ memory\r\n");


	ArmEnableMmu();
	ArmEnableDataCache();
	ArmEnableInstructionCache();

	if (Something == 0xFFFFFFFF) {
		uart_print("Something happened and the exploit doesn't work\r\n");
		Status = EFI_ABORTED;
	}*/

	uint8_t* pMem = (uint8_t*)0x80000C00;
	uint32_t size = 0x00000100;

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

	/*
	uart_print("\r\n\r\n");
	uart_print("Jumping to COPY payload!\r\n");
	address = PointerToInt(dest);
	address = 0x83000000;
	void (*func_ptr2)(void) = (void (*)())address;
	func_ptr2();

	uart_print("We are back to UEFI!");	
	*/


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

/*
	Dump memory to a file on the EFI partition this program booted from
	@param data	Base memory address where to start dumping from
	@param size How man bytes to be dumped starting from the base memory address
*/
EFI_STATUS memdump_to_file(const uint8_t *data, uint32_t size) {

    //EFI_STATUS status = EFI_SUCCESS;
	/*EFI_FILE_PROTOCOL* file = NULL;
	EFI_LOADED_IMAGE *loaded_image = NULL;
    EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
	EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* volumeHandle = NULL;
	EFI_FILE_PROTOCOL* rootDirectory = NULL;*/

	char printBuffer[16];
	uart_print("1st debug point \r\n");
	uart_print(itoa((int)data, printBuffer, 16));
	uart_print("\r\n");
	uart_print(itoa((int)size, printBuffer, 16));
	uart_print("\r\n");

	/*// Get the loaded image handle
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

	uart_print("2nd debug point \r\n");
	uart_print(itoa((int)data, printBuffer, 16));
	uart_print("\r\n");
	uart_print(itoa((int)size, printBuffer, 16));
	uart_print("\r\n");*/

	// Our desired file is now open. At this point writing to it is possible, but that data still needs to be prepared.

	// Setup a buffer where we can copy trustzone memory to.
//	void* memoryDumpBuffer = NULL;
	void* memoryDumpBuffer = (void*)0x83000000;

	/*status = gBS->AllocatePool(
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
	}*/

	uart_print("3rd debug point \r\n");
	uart_print(itoa((int)data, printBuffer, 16));
	uart_print("\r\n");
	uart_print(itoa((int)size, printBuffer, 16));
	uart_print("\r\n");

	

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
	//Print(L"anything\n");

	ArmDisableCachesAndMmu();

	/*void* dest = memoryDumpBuffer;
	const void* src = data;
	size_t l_size = size;*/

	uart_print("4th debug point \r\n");
	uart_print(itoa((int)data, printBuffer, 16));
	uart_print("\r\n");
	uart_print(itoa((int)size, printBuffer, 16));
	uart_print("\r\n");

	/*uart_print("5th debug point \r\n");
	uart_print(itoa((int)dest, printBuffer, 16));
	uart_print("\r\n");
	uart_print(itoa((int)src, printBuffer, 16));
	uart_print("\r\n");
	uart_print(itoa((int)l_size, printBuffer, 16));
	uart_print("\r\n");*/

	memcpy_usr(memoryDumpBuffer, (const void*)data, (size_t)size);
//	memcpy_usr(dest, src, l_size);

	ArmEnableMmu();
	ArmEnableDataCache();
	ArmEnableInstructionCache();	

	uart_print("6th debug point \r\n");
	uart_print(itoa((int)data, printBuffer, 16));
	uart_print("\r\n");
	uart_print(itoa((int)size, printBuffer, 16));
	uart_print("\r\n");

	/*uart_print("Writing memory!\r\n");

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
	
	uart_print("Closed file. USB can now be unplugged.\r\n");*/

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