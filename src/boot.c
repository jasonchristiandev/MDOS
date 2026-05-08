#include "MDOS/bootinfo.h"
#include <efi.h>
#include <efilib.h>

#define EXIT                                                                                  \
	UINTN exit_index;                                                                         \
	EFI_INPUT_KEY exit_key;                                                                   \
	LOG_INFO(L"Press any key to exit...\r\n");                                                \
	SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey, &exit_index); \
	SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &exit_key);                         \
	LOG_INFO(L"Exiting...\r\n\r\n");

#define LOG_INFO(format, ...)                                                                    \
	SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK)); \
	Print(L"[INFO] " format, ##__VA_ARGS__)

#define LOG_WARNING(format, ...)                                                                  \
	SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_BLACK, EFI_YELLOW)); \
	Print(L"[WARNING] " format, ##__VA_ARGS__);                                                   \
	SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK))

#define LOG_ERROR(format, ...)                                                                 \
	SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_RED)); \
	Print(L"[ERROR] " format, ##__VA_ARGS__);                                                  \
	SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK))

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	// Initialization

	InitializeLib(ImageHandle, SystemTable);

	SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

	LOG_INFO(L"Starting MDOS...\r\n");

	// Load Kernel

	EFI_LOADED_IMAGE *image = NULL;
	SystemTable->BootServices->HandleProtocol(
		ImageHandle,
		&gEfiLoadedImageProtocolGuid,
		(void **) &image);

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *file_system = NULL;
	SystemTable->BootServices->HandleProtocol(
		image->DeviceHandle,
		&gEfiSimpleFileSystemProtocolGuid,
		(void **) &file_system);

	EFI_FILE *root = NULL;
	file_system->OpenVolume(file_system, &root);

	EFI_FILE *kernel_file = NULL;
	EFI_STATUS kernel_file_status = root->Open(
		root,
		&kernel_file,
		L"kernel.bin",
		EFI_FILE_MODE_READ,
		EFI_FILE_READ_ONLY);

	if (kernel_file_status != EFI_SUCCESS) {
		LOG_ERROR(L"Failed to load kernel.bin.\r\n");
		EXIT;
		return kernel_file_status;
	}

	LOG_INFO(L"Found kernel.bin on the disk.\r\n");

	EFI_FILE_INFO *file_info = NULL;
	UINTN file_info_size = sizeof(EFI_FILE_INFO) + 256;
	SystemTable->BootServices->AllocatePool(EfiLoaderData, file_info_size, (void **) &file_info);

	EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
	kernel_file->GetInfo(kernel_file, &file_info_guid, &file_info_size, file_info);

	UINTN kernel_size = file_info->FileSize;
	SystemTable->BootServices->FreePool(file_info);

	void *kernel_buffer = NULL;
	kernel_file_status = SystemTable->BootServices->AllocatePool(EfiLoaderCode, kernel_size, &kernel_buffer);

	if (kernel_file_status != EFI_SUCCESS) {
		LOG_ERROR(L"Failed to allocate memory for kernel.\r\n");
		EXIT;
		return kernel_file_status;
	}

	LOG_INFO(L"Allocated memory for kernel (%d bytes).\r\n", kernel_size);

	kernel_file_status = kernel_file->Read(kernel_file, &kernel_size, kernel_buffer);

	if (kernel_file_status != EFI_SUCCESS) {
		LOG_ERROR(L"Failed to read kernel data into memory.\r\n");
		EXIT;
		return kernel_file_status;
	}

	LOG_INFO(L"Successfully loaded kernel into memory at address 0x%x\r\n", kernel_buffer);

	kernel_file->Close(kernel_file);

	// Graphics

	EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
	EFI_STATUS gop_status = SystemTable->BootServices->LocateProtocol(
		&gEfiGraphicsOutputProtocolGuid,
		NULL,
		(void **) &Gop);

	if (gop_status != EFI_SUCCESS) {
		LOG_ERROR(L"Failed to locate GOP (Graphics Output Protocol).\r\n");
		return gop_status;
	}

	LOG_INFO(L"Graphics initialized (%dx%d).\r\n",
			 Gop->Mode->Info->HorizontalResolution,
			 Gop->Mode->Info->VerticalResolution);

	BOOT_INFO BootInfo;
	BootInfo.framebuffer_base = (void *) Gop->Mode->FrameBufferBase;
	BootInfo.framebuffer_size = Gop->Mode->FrameBufferSize;
	BootInfo.width = Gop->Mode->Info->HorizontalResolution;
	BootInfo.height = Gop->Mode->Info->VerticalResolution;
	BootInfo.pixels_per_scanline = Gop->Mode->Info->PixelsPerScanLine;

	// Start Kernel

	typedef void __attribute__((sysv_abi)) (*KernelEntry)(BOOT_INFO *);
	KernelEntry kernel_start = (KernelEntry) kernel_buffer;

	LOG_INFO(L"Starting kernel...");

	kernel_start(&BootInfo);

	return EFI_SUCCESS;
}