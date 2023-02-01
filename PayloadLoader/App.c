#include "Include/Application.h"
#include <Library/BaseCryptLib.h>
#include <Library/MemoryAllocationLib.h>

EFI_STATUS loadPayloadIntoMemory(EFI_PHYSICAL_ADDRESS memoryAddress, short unsigned int fileName[], size_t* fileSize);

void memcpy_usr(void* dest, const void* src, size_t n) {
	char *src_char = (char *)src;
	char *dest_char = (char *)dest;
	for (int i=0; i<n; i++) {
		dest_char[i] = src_char[i]; //copy contents byte by byte
	}
}

EFI_STATUS calc_sha256 (
		UINT8 		*memory,
		IN  UINTN 	length,
		OUT UINT8 	*response
	)
{
	// Example for calculation a Hash
	/*
	UINT8 hash_this[8] = "ABCDEFGH";
	UINT8 hash[SHA256_DIGEST_SIZE];
	calc_sha256(hash_this, 8, hash);
	Print(L"Hash:");
	for (int i = 0; i < 16; i++) {
		Print(L"Payloadsize: %02x", hash[i]);
	}
	Print(L"\n");
	*/

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
		uart_print("%02x", Digest[i]);
	}
	Print(L"\n");
	uart_print("\n");

exit:
	FreePool (HashContext);

	return EFI_SUCCESS;
}

EFI_STATUS PayloadLoaderEntryPoint(
		EFI_HANDLE 		ImageHandle,
		EFI_SYSTEM_TABLE 	*SystemTable
	)
{
	EFI_STATUS Status = EFI_SUCCESS;

	size_t file_size_primary_smc = 0;
	size_t file_size_secondary_smc = 0;
	size_t file_size_secondary_trampoline = 0;
	//size_t file_size_uboot = 0;

	const uint32_t addr_primary_smc = 0x83000000U;
	const uint32_t addr_secondary_smc = 0x83005000U;
	const uint32_t addr_secondary_trampoline = 0x8300A000U;

	uart_print("addr_secondary_smc: %016x\r\n", addr_secondary_smc);
	//const EFI_PHYSICAL_ADDRESS addr_uboot = 0x84000000U;

	// Turn off watchdog timer, since this does take a while
	uart_print("Disable Watchdog\r\n");
	gBS->SetWatchdogTimer(0, 0, 0, NULL);

	// Fix Surface RT UEFI on-screen console
	Tegra3ConsoleOutputFixup();
	Print(L"We do something now\n");
	uart_print("UEFI: Start the fun :)\r\n");

	// Unprotect Trustzone
	uart_print("Unprotect TZ\r\n");
	SurfaceRTExploit();
	uart_print("Unprotect TZ: done\r\n");

	// Load Primary SMC Handler payload into memory
	uart_print("Primary SMC Handler: Load\r\n");
	Status = loadPayloadIntoMemory((EFI_PHYSICAL_ADDRESS)addr_primary_smc, L"\\primary.bin", &file_size_primary_smc);
	if (Status != EFI_SUCCESS)
	{
		uart_print("Primary SMC Handler: Loading Failed\r\n");
		FinalizeApp();
	}
	uart_print("Primary SMC Handler: Loaded Successfully\r\n");

	// Load Secondary SMC Handler payload into memory
	uart_print("Secondary SMC Handler: Load\r\n");
	Status = loadPayloadIntoMemory((EFI_PHYSICAL_ADDRESS)addr_secondary_smc, L"\\secondary.bin", &file_size_secondary_smc);
	if (Status != EFI_SUCCESS)
	{
		uart_print("Secondary SMC Handler: Loading Failed\r\n");
		FinalizeApp();
	}
	uart_print("Secondary SMC Handler: Loaded Successfully\r\n");

	// Load start Secondary Trampoline
	uart_print("Secondary Trampoline: Load\r\n");
	Status = loadPayloadIntoMemory((EFI_PHYSICAL_ADDRESS)addr_secondary_trampoline, L"\\secondary_trampolin.bin", &file_size_secondary_trampoline);
	if (Status != EFI_SUCCESS)
	{
		uart_print("Secondary Trampoline: Loading Failed\r\n");
		FinalizeApp();
	}
	uart_print("Secondary Trampoline: Loaded Successfully\r\n");

	// Put uboot into memory
	/*
	uart_print("U-Boot: Load\r\n");
	Status = loadPayloadIntoMemory(addr_uboot, L"\\u-boot-dtb.bin", &file_size_uboot);
	if (Status != EFI_SUCCESS)
	{
		uart_print("U-Boot: Loading Failed\r\n");
		FinalizeApp();
	}
	uart_print("U-Boot: Loaded Successfully\r\n");
	*/

	// Disable MMU to get access to Trustzone memory by disarming the Translation Table / Page Table
	// NO UEFI FROM HERE: Print(), ..., and so on
	ArmDisableFiq();
	ArmDisableInterrupts();
	ArmDisableCachesAndMmu();
	uart_print("UEFI: MMU disabled\r\n");

	// Copy payload into Trustzone memory.
	// 0x8011216C is in the SMC handler
	// The memory needs to be marked as secure, as you can only execute secure memory in secure mode so
	// we copy the payload to TZ memory
	//memcpy_usr((void*)(0x8011216C), (const void*)addr_primary_smc, file_size_primary_smc);
	memcpy_usr((void*)(0x82002820), (const void*)addr_secondary_smc, file_size_secondary_smc);
	ArmDataSynchronizationBarrier();
	uart_print("UEFI: payload copied\r\n");

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

	uart_print("I just wrote Trustzone and I'm still alive!\r\n");


	uart_print("start secondary core\r\n");
	*((volatile uint32_t*)0x88008000U) = 0;
	start_secondary_core(1);
	volatile uint32_t memoryvalue = 0;
	volatile uint32_t oldval = 0;
	do{
		memoryvalue = mem_read((uint32_t)0x88008000U);
		if (memoryvalue != oldval) {
			uart_print("memread 0x88008000:%u\r\n",memoryvalue);
			oldval = memoryvalue;
		}
	}while(1);

	// Say goodbye to UEFI.
	uart_print("UEFI part finished. Setting up for SMC.\n");
	uart_print("See you soon\r\n");




/*


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

        // Shutdown 
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



	while(1);
*/
	// This should trigger an SMC, jump to the payload and output stuff to uart. Hopefully.
	ArmCallSmcHelper(0, 0, 0, 0);

	// We shouldn't get here since going back from the SMC in the payload isn't implemented and probably won't.
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


// adopted from kernel code. thanks Leander :)
struct parking_protocol_mailbox {
	uint32_t cpu_id;
	uint32_t reserved;
	uint64_t entry_point; // keep at 64Bit to keep cpu_mailbox_entry aligned
};

struct cpu_mailbox_entry {
	struct parking_protocol_mailbox *mailbox;
	uint32_t mailbox_addr;
	uint8_t version;
	uint8_t gic_cpu_id;
};

static struct cpu_mailbox_entry cpu_mailbox_entries[4];

// removed 'unsigned int cpu' from parameter list and changed return type to void
static void acpi_parking_protocol_cpu_init(void)
{
	//Print(L"%s: has been called. Hardcoding MADT table for Surface RT.\r\n", __func__);

	cpu_mailbox_entries[0].gic_cpu_id = 0;
	cpu_mailbox_entries[0].version = 1;
	cpu_mailbox_entries[0].mailbox_addr = 0x82001000;
	cpu_mailbox_entries[0].mailbox = (struct parking_protocol_mailbox*)(0x82001000U);

	cpu_mailbox_entries[1].gic_cpu_id = 1;
	cpu_mailbox_entries[1].version = 1;
	cpu_mailbox_entries[1].mailbox_addr = 0x82002000;
	cpu_mailbox_entries[1].mailbox = (struct parking_protocol_mailbox*)(0x82002000U);

	cpu_mailbox_entries[2].gic_cpu_id = 2;
	cpu_mailbox_entries[2].version = 1;
	cpu_mailbox_entries[2].mailbox_addr = 0x82003000;
	cpu_mailbox_entries[2].mailbox = (struct parking_protocol_mailbox*)(0x82003000U);

	cpu_mailbox_entries[3].gic_cpu_id = 3;
	cpu_mailbox_entries[3].version = 1;
	cpu_mailbox_entries[3].mailbox_addr = 0x82004000;
	cpu_mailbox_entries[3].mailbox = (struct parking_protocol_mailbox*)(0x82004000U);
}

void start_secondary_core(uint32_t cpu) {
	acpi_parking_protocol_cpu_init();

	//Print(L"Let's goooo\r\n");

	//Print(L"mailbox_address: %p\r\n", &cpu_mailbox_entries[cpu].mailbox->cpu_id);

	//Print(L"mailbox_value: %08x\r\n", *((uint32_t*)0x82002000U));

	uint32_t cpu_id = *((uint32_t*)0x82002000U); //mem_read(cpu_mailbox_entries[cpu].mailbox->cpu_id);
	//Print(L"cpu: %d\r\n", cpu);
	//Print(L"cpu_id: %d\r\n", cpu_id);

	if (cpu_id != ~0U) {
		Print(L"something wrong\r\n");
	}

	// Let the secondary core use the payload loaded by UEFI.
	//Print(L"entry_write: %p\r\n", (uint32_t)(&cpu_mailbox_entries[cpu].mailbox->entry_point));
	//mem_write((uint32_t)(&cpu_mailbox_entries[cpu].mailbox->entry_point), 0x83800000U);
	*((uint32_t*)0x82002008) = 0x8300A000U;
	//Print(L"cpu_write: %p\r\n", (uint32_t)(&cpu_mailbox_entries[cpu].mailbox->cpu_id));
	//mem_write((uint32_t)(&cpu_mailbox_entries[cpu].mailbox->cpu_id), cpu);
	*((uint32_t*)0x82002000) = 0x00000001U;
	// Interrupt magic.
	// interrupt according to ACPI PP 0x00fe0000
	// reg: 0xf00
	// base: 0x50041000

	//Print(L"mailbox_cpu_id: %08x\r\n", *((uint32_t*)0x82002000U));
	//Print(L"mailbox_entry: %08x\r\n", *((uint32_t*)0x82002008U));

	//set_ns();

	//Print(L"now the interrupt\r\n");
	//mem_write(0x50041f00U, 0x00fe0000U);
	asm volatile("dmb ishst\n");
	// use ArmGicSendSgiTo : ArmPkg/Drivers/ArmGiC/ArnGicLib.c
	// Does the same thing boot looks cooler ;)
	*((uint32_t*)0x50041f00U) = 0x00fe0000U;

	uart_print("Waiting for Mailbox to clear\r\n");
	volatile uint32_t reg = mem_read(0x82002008);
	while (reg) {
		reg = mem_read(0x82002008);
	}
	uart_print("Mailbox cleared\r\n");

}
