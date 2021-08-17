#include "Include/Application.h"

EFI_STATUS memdump_to_file(const uint8_t *data, uint32_t size);

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

EFI_STATUS MemoryDumpToolEntryPoint(
    EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS Status = EFI_SUCCESS;
	
	// Turn off watchdog timer, since this does take a while
	gBS->SetWatchdogTimer(0, 0, 0, NULL);

	Tegra3ConsoleOutputFixup();

	Print(L"Hello World\n");
	uart_print("Hello World.\r\n");

	uart_print("perform exploit\r\n");
	PerformNvTegra3Exploit();
	uart_print("done exploit\r\n");

	// Select what to dump

	uint8_t* pMem = (uint8_t*)0x80000000; // start of trustzone 
	//uint8_t* pMem = (uint8_t*)0xFB49A000; // start of high memory, also used by UEFI
	uint32_t size = 0x03000000; // covers the whole trustzone + some other memory
	//uint32_t size = 0x04A45FFF; // Address 0xFFEDFFFF; end of high memory

	// Dump memory to file

	Print(L"Start dumping to file! This may take a few minutes.\n");
	uart_print("Start dumping to file! This may take a few minutes.\r\n");

	Status = memdump_to_file(pMem, size);
	
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