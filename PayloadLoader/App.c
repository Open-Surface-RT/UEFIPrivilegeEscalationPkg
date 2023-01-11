﻿#include "Include/Application.h"
//#include <Library/BaseCryptLib.h>
#include <Library/BaseCryptLib.h>
//#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

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

EFI_STATUS calc_sha256 (
    UINT8   *memory,
    IN  UINTN    length,
    OUT UINT8   *response
  )
{
	VOID     *HashContext;
	UINT8    Digest[SHA256_DIGEST_SIZE];

	HashContext = AllocatePool (Sha256GetContextSize ());
	ZeroMem (Digest, SHA256_DIGEST_SIZE);

	if (!Sha256Init (HashContext)) {
		goto exit;
	}

	if (!Sha256Update (HashContext, memory, length)) {
		goto exit;
	}

	if (!Sha256Final (HashContext, Digest)) {
		goto exit;
	}

	Print(L"Hash:");
	for (int i = 0; i < SHA256_DIGEST_SIZE; i++) {
		Print(L"%02x", Digest[i]);
	}
	Print(L"\n");

exit:
	FreePool (HashContext);

	return EFI_SUCCESS;
}

EFI_STATUS PayloadLoaderEntryPoint(
    EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS Status = EFI_SUCCESS;
	
	// Turn off watchdog timer, since this does take a while
	Print(L"Disable Watchdog\n");
	gBS->SetWatchdogTimer(0, 0, 0, NULL);

	Print(L"Fixing T30 Console\n");
	Tegra3ConsoleOutputFixup();
	
	Print(L"Payload Loader");
	Print(L"\tHello World\n");
	uart_print("Hello World.\r\n");

	UINT8 hash_this[8] = "ABCDEFGH";
	UINT8 hash[SHA256_DIGEST_SIZE];
	calc_sha256(hash_this, 8, hash);
	Print(L"Hash:");
	for (int i = 0; i < 16; i++) {
		Print(L"Payloadsize: %02x", hash[i]);
	}
	Print(L"\n");

	while(1);

	uart_print("perform exploit\r\n");
	Print(L"\tUnprotect TZ\n");
	PerformNvTegra3Exploit();
	uart_print("done exploit\r\n");

	//
	// Put payload into memory
	//
	Print(L"\tPutting payload into memory!\n");
	uart_print("Putting payload into memory!\r\n");

	size_t fileSize1 = 0;
	Status = loadPayloadIntoMemory((EFI_PHYSICAL_ADDRESS)0x83000000, L"\\payload.bin", &fileSize1);
	
	if (Status != EFI_SUCCESS)
	{
		Print(L"\tFailed at loading payload!\n");
		uart_print("Failed at loading payload!\r\n");
		CpuDeadLoop();
	}

	Print(L"\tPayload is now in memory!\n");
	uart_print("Payload is now in memory!\r\n");

	char printBuffer[10];
	uart_print("Printing the 1st 4 bytes at 0x83000000!\r\n");
	uart_print(utoa(*((uint32_t*)(0x83000000)), printBuffer, 16));
	uart_print("\r\n");

	//
	// Put uboot into memory
	//
	Print(L"\tPutting uboot into memory!\n");
	uart_print("Putting uboot into memory!\r\n");

	size_t fileSize2 = 0;
	Status = loadPayloadIntoMemory((EFI_PHYSICAL_ADDRESS)0x84000000, L"\\u-boot-dtb.bin", &fileSize2);
	
	if (Status != EFI_SUCCESS)
	{
		Print(L"\tFailed at loading uboot!\n");
		uart_print("Failed at loading uboot!\r\n");
		CpuDeadLoop();
	}

	Print(L"\tUboot is now in memory!\n");
	uart_print("Uboot is now in memory!\r\n");

	uart_print("Printing the 1st 4 bytes at 0x84000000!\r\n");
	uart_print(utoa(*((uint32_t*)(0x84000000)), printBuffer, 16));
	uart_print("\r\n");

	// Disable MMU to get access to Trustzone memory
	ArmDisableCachesAndMmu();

	// Copy payload into Trustzone memory. 
	// 0x80112174 is in the SMC handler, right after the Synchronization barriers
	// The memory needs to be marked as secure, as you can only execute secure memory in secure mode so
	// we copy the payload to TZ memory
	memcpy_usr((void*)(0x80112174), (const void*)0x83000000, (size_t)fileSize1);
	memcpy_usr((void*)(0x81000000), (const void*)0x84000000, (size_t)fileSize2);
	
	uart_print("Printing the 1st 4 bytes at 0x81000000!\r\n");
	uart_print(utoa(*((uint32_t*)(0x81000000)), printBuffer, 16));
	uart_print("\r\n");
	//Print(L"I just wrote Trustzone and I'm still alive!\r\n");
	
	// Payload is now in place. Enable MMU for memory dump.
	ArmEnableMmu();
	ArmEnableDataCache();
	ArmEnableInstructionCache();
	
	uart_print("I just wrote Trustzone and I'm still alive!\r\n");

	Print(L"\tUEFI part finished. Setting up for SMC...\n");
	Print(L"\tSee you on the otherside.\n");
	// This should trigger an SMC, jump to the payload and output stuff to uart. Hopefully.
	uart_print("See you soon\r\n");
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
