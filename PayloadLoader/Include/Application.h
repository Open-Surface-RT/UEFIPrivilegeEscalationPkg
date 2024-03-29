// Copyright (c) 2019 - 2020, Bingxing Wang and other project authors. All rights reserved.<BR>
// Copyright (c) 2021 - 2021, Leander Wollersberger. All rights reserved.<BR>

#include <Uefi.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/FileHandleLib.h>
#include <Protocol/LoadedImage.h>
#include <Guid/FileInfo.h>
#include <Library/BaseCryptLib.h>

#include <Protocol/SimpleTextOut.h>

#include <stdint.h>

void uart_print(IN  CONST CHAR8  *FormatString, ...);
void uart_ll_print(char* buf);
void uart_init();

void start_secondary_core(int cpu);

EFI_STATUS calc_sha256 (
		UINT8 		*memory,
		IN  UINTN 	length,
		OUT UINT8 	*response
	);

typedef UINTN  size_t;
typedef UINT8  uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;

typedef struct _VERSION_TABLE_ENTRY {
  CHAR16 *FirmwareRelease;
  VOID *  EntryPoint;
  VOID *  PreEntryFixup;
} VERSION_TABLE_ENTRY, *PVERSION_TABLE_ENTRY;

typedef void (*HACK_ENTRY)(void);

// Routines
VOID SurfaceRTExploit(VOID);
VOID FinalizeApp(VOID);

VOID PerformNvTegra3Exploit(VOID);

UINT32 ArmCallSmcHelper(UINT32 R0, UINT32 R1, UINT32 R2, UINT32 R3);

VOID Tegra3ConsoleOutputFixup(VOID);

EFI_STATUS LaunchExploitByVersionTable(VOID);

void *memmem(const void *h0, size_t k, const void *n0, size_t l);
#define memchr(buf, ch, count) ScanMem8(buf, (UINTN)(count), (UINT8)ch)
#define memcmp(buf1, buf2, count) (int)(CompareMem(buf1, buf2, (UINTN)(count)))

#define _MAX(a, b) ((a) > (b) ? (a) : (b))
#define _MIN(a, b) ((a) < (b) ? (a) : (b))

#define BITOP(a, b, op)                                                        \
  ((a)[(size_t)(b) / (8 * sizeof *(a))] op(size_t) 1                           \
   << ((size_t)(b) % (8 * sizeof *(a))))

extern EFI_GUID gEfiGlobalVariableGuid;


#define _MEM(addr) *(volatile uint32_t *)(addr)
#define mem_read(addr) _MEM(addr)
#define mem_write(addr, value) _MEM(addr) = value
#define mem_clear(base, value) _R_MEMEG(addr) &= ~value
#define mem_set(base, value) _RE_MEMG(addr) |= value
