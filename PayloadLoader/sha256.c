#include "Include/Application.h"

#include <Library/MemoryAllocationLib.h>

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
