#include "Include/Application.h"
#include <Library/MemoryAllocationLib.h>

EFI_STATUS loadPayloadIntoMemory(EFI_PHYSICAL_ADDRESS memoryAddress, short unsigned int fileName[], size_t* fileSize);

void memcpy_usr(void* dest, const void* src, size_t n) {
	char *src_char = (char *)src;
	char *dest_char = (char *)dest;
	for (int i=0; i<n; i++) {
		dest_char[i] = src_char[i]; //copy contents byte by byte
	}
}

EFI_STATUS PayloadLoaderEntryPoint(
		EFI_HANDLE 		ImageHandle,
		EFI_SYSTEM_TABLE 	*SystemTable
	)
{
	EFI_STATUS Status = EFI_SUCCESS;

	// Turn off watchdog timer, since this does take a while
	Print(L"Disable Watchdog\n");
	gBS->SetWatchdogTimer(0, 0, 0, NULL);

	// Fix Surface RT UEFI on-screen console
	Print(L"Fixing T30 Console. You can't see me ;)\n");
	Tegra3ConsoleOutputFixup();
	Print(L"Welcome to the Payload Loader\n");
	uart_print("UEFI: Payload Loader\r\n");

	// Unprotect Trustzone
	Print(L"\tUnprotect TZ\n");
	uart_print("Unprotect TZ\r\n");

	SurfaceRTExploit();

	uart_print("done exploit\r\n");
	Print(L"done exploit\n");

	// Load exploit payload into memory
	Print(L"Loading exploit payload into memory!\n");
	uart_print("Loading exploit payload into memory!\r\n");
	size_t fileSize1 = 0;
	Status = loadPayloadIntoMemory((EFI_PHYSICAL_ADDRESS)0x83000000, L"\\s_primary.bin", &fileSize1);
	if (Status != EFI_SUCCESS)
	{
		Print(L"\tFailed at loading payload!\n");
		uart_print("Failed at loading payload!\r\n");
		FinalizeApp();
	}
	Print(L"\tPayload is now in memory!\n");
	uart_print("Payload is now in memory!\r\n");

        // Load exploit payload into memory
        Print(L"Loading exploit payload into memory!\n");
        uart_print("Loading exploit payload into memory!\r\n");
        size_t fileSize1b = 0;
        Status = loadPayloadIntoMemory((EFI_PHYSICAL_ADDRESS)0x83100000, L"\\s_secondary.bin", &fileSize1b);
        if (Status != EFI_SUCCESS)
        {
                Print(L"\tFailed at loading payload!\n");
                uart_print("Failed at loading payload!\r\n");
                FinalizeApp();
        }
        Print(L"\tPayload is now in memory!\n");
        uart_print("Payload is now in memory!\r\n");

	// Load exploit for secondary core into memory
	Print(L"Loading secondary payload into memory!\n");
	uart_print("Loading secondary payload into memory!\r\n");
	size_t fileSize3 = 0;
	Status = loadPayloadIntoMemory((EFI_PHYSICAL_ADDRESS)0x83800000, L"\\secondary_trampolin.bin", &fileSize3);
	if (Status != EFI_SUCCESS)
 	{
		Print(L"\tFailed at loading secondary payload!\n");
		uart_print("Failed at loading secondary payload!\r\n");
		FinalizeApp();
	}
	Print(L"\tSecondary is now in memory!\n");
	uart_print("Secondary is now in memory!\r\n");

	// Put uboot into memory
	Print(L"Loading u-boot into memory!\n");
	uart_print("Loading u-boot into memory!\r\n");
	size_t fileSize2 = 0;
	Status = loadPayloadIntoMemory((EFI_PHYSICAL_ADDRESS)0x84000000, L"\\u-boot-dtb.bin", &fileSize2);
	if (Status != EFI_SUCCESS)
	{
		Print(L"\tFailed at loading u-boot!\n");
		uart_print("Failed at loading u-boot!\r\n");
		FinalizeApp();
	}
	Print(L"\tU-boot is now in memory!\n");
	uart_print("U-boot is now in memory!\r\n");

	// Disable MMU to get access to Trustzone memory by disarming the Translation Table / Page Table
	// NO UEFI FROM HERE: Print(), ..., and so on
	ArmDisableFiq();
	ArmDisableInterrupts();
	ArmDisableCachesAndMmu();
	uart_print("UEFI: MMU disbaled\r\n");

	// Copy payload into Trustzone memory.
	// 0x80112174 is in the SMC handler, right after the Synchronization barriers
	// The memory needs to be marked as secure, as you can only execute secure memory in secure mode so
	// we copy the payload to TZ memory

	memcpy_usr((void*)(0x80112174), (const void*)0x83000000, (size_t)fileSize1);

	// Redirect Core 1 SMC handler to the secure payload location
	// Could be done with MMU enabled
	// Since the copy with MMU disabled often fails it might be better to do it outside?
	mem_write(0x82002880U, 0x83100000U);

	ArmDataSynchronizationBarrier();
	uart_print("UEFI: payload copied\r\n");
	// Don't copy U-Boot as we can execute from everywhere in Secure World once TTB is disarmed
	//memcpy_usr((void*)(0x81000000), (const void*)0x84000000, (size_t)fileSize2);

	// Payload is now in place. Enable MMU to use UEFI one last time
	ArmEnableMmu();
	uart_print("UEFI: MMU enabled\r\n");
	ArmEnableDataCache();
	uart_print("UEFI: D-Cache enabled\r\n");
	ArmEnableInstructionCache();
	uart_print("UEFI: I-Cache enabled\r\n");
	// UEFI funtions can be used again.
	ArmEnableFiq();
	ArmEnableInterrupts();

	Print(L"I just wrote Trustzone and I'm still alive!\r\n");
	uart_print("I just wrote Trustzone and I'm still alive!\r\n");

	// Say goodbye to UEFI.
	Print(L"\tUEFI part finished. Setting up for SMC.\n");
	uart_print("UEFI part finished. Setting up for SMC.\n");
	Print(L"\tSee you on the otherside.\n");
	uart_print("See you soon\r\n");

	mem_write(0x88000000, 0);

	start_secondary_core(1);

	// EXIT BOOT SERVICE AS TEST
	UINTN MemMapSize = 0;
	EFI_MEMORY_DESCRIPTOR* MemMap = 0;
	UINTN MapKey = 0;
	UINTN DesSize = 0;
	UINT32 DesVersion = 0;

	// May pass some parameters if needed?
	gBS->GetMemoryMap(
                &MemMapSize,
                MemMap,
                &MapKey,
                &DesSize,
                &DesVersion
        );

        /* Shutdown */
        Status = gBS->ExitBootServices(
                ImageHandle,
                MapKey
        );

        if (EFI_ERROR(Status))
        {
                Print(L"Failed to exit BS\n");
                uart_print("BootService NOT gone ;(\r\n");
        }
	// EXIT BOOT SERVICE AS TEST

//	while(1);

	// This should trigger an SMC, jump to the payload and output stuff to uart. Hopefully.
	ArmCallSmcHelper(0, 0, 0, 0);

	// We shouldn't get here since going back from the SMC in the payload isn't implemented and probably won't.
	Print(L"Something went wrong, we shouldn't be here\n");
	uart_print("Something went wrong, we shouldn't be here\r\n");

	// We already messed with UEFI so we can't continue normally.
	FinalizeApp();
	return Status;
}

VOID FinalizeApp(VOID)
{
	// Let people wait for stroke
	uart_print("!!! PLEASE RESET YOUR DEVICE MANUALLY USING THE POWER BUTTON !!!\n");
	uart_print("!!! PLEASE RESET YOUR DEVICE MANUALLY USING THE POWER BUTTON !!!\n");
	uart_print("!!! PLEASE RESET YOUR DEVICE MANUALLY USING THE POWER BUTTON !!!\n");
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

	uart_print("memoryAddress: %016lx\r\n", memoryAddress);
	EFI_PHYSICAL_ADDRESS payloadFileBuffer = memoryAddress;
	uart_print("payloadFileBuffer: %016lx\r\n", payloadFileBuffer);

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

	Print(L"File is now in memory at location 0x%08lx!\n", payloadFileBuffer);

	// Calc hash of loaded data
	UINT8    Digest[SHA256_DIGEST_SIZE];
	calc_sha256 (
		(uint8_t*)payloadFileBuffer,
		payloadFileSize,
		Digest
	);

	//uart_print("Printing the 1st 4 bytes at 0x84000000: %08x\r\n", *((uint32_t*)(0x84000000)));

	return EFI_SUCCESS;
}
