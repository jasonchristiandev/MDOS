#include "MDOS/bootinfo.h"
#include "MDOS/utils.h"
#include "efilib.h"
#include <efi.h>

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
	// Initialization

	system_table->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
	system_table->ConOut->ClearScreen(system_table->ConOut);

	LOG_INFO(L"BOOTLOADER", L"Starting MDOS...\r\n");

	// Load Kernel

	EFI_LOADED_IMAGE *image = NULL;
	system_table->BootServices->HandleProtocol(
		image_handle,
		&gEfiLoadedImageProtocolGuid,
		(void **) &image);

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *file_system = NULL;
	system_table->BootServices->HandleProtocol(
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
		LOG_ERROR(L"BOOTLOADER", L"Failed to load kernel.bin.\r\n");
		EXIT(L"KERNEL");
		return kernel_file_status;
	}

	LOG_INFO(L"BOOTLOADER", L"Found kernel.bin on the disk.\r\n");

	EFI_FILE_INFO *file_info = NULL;

	UINTN file_info_size = sizeof(EFI_FILE_INFO) + 256;
	system_table->BootServices->AllocatePool(EfiLoaderData, file_info_size, (void **) &file_info);

	EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
	kernel_file->GetInfo(kernel_file, &file_info_guid, &file_info_size, file_info);

	UINTN kernel_size = file_info->FileSize;
	EFI_PHYSICAL_ADDRESS kernel_address = 0x100000;
	UINTN pages = (kernel_size / 4096) + 1;

	kernel_file_status = system_table->BootServices->AllocatePages(
		AllocateAddress,
		EfiLoaderCode,
		pages,
		&kernel_address);

	if (kernel_file_status != EFI_SUCCESS) {
		LOG_ERROR(L"BOOTLOADER", L"Failed to reserve 1MB mark for kernel.\r\n");
		EXIT(L"KERNEL");
		return kernel_file_status;
	}
	LOG_INFO(L"BOOTLOADER", L"Reserved 1MB mark for kernel.\r\n");

	void *kernel_buffer = (void *) kernel_address;

	kernel_file_status = kernel_file->Read(kernel_file, &kernel_size, kernel_buffer);

	if (kernel_file_status != EFI_SUCCESS) {
		LOG_ERROR(L"BOOTLOADER", L"Failed to read kernel data into memory.\r\n");
		EXIT(L"KERNEL");
		return kernel_file_status;
	}

	LOG_INFO(L"BOOTLOADER", L"Successfully loaded kernel into memory at address 0x%x!\r\n", kernel_buffer);

	kernel_file->Close(kernel_file);

	// Graphics

	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
	EFI_STATUS gop_status = system_table->BootServices->LocateProtocol(
		&gEfiGraphicsOutputProtocolGuid,
		NULL,
		(void **) &gop);

	if (gop_status != EFI_SUCCESS) {
		LOG_ERROR(L"BOOTLOADER", L"Failed to locate GOP (Graphics Output Protocol).\r\n");
		return gop_status;
	}

	LOG_INFO(L"BOOTLOADER", L"Graphics found (%dx%d).\r\n",
			 gop->Mode->Info->HorizontalResolution,
			 gop->Mode->Info->VerticalResolution);

	BOOT_INFO boot_info;
	boot_info.system_table = system_table;
	GRAPHICS_INFO graphics_info;
	graphics_info.framebuffer_base = (void *) gop->Mode->FrameBufferBase;
	graphics_info.framebuffer_size = gop->Mode->FrameBufferSize;
	graphics_info.width = gop->Mode->Info->HorizontalResolution;
	graphics_info.height = gop->Mode->Info->VerticalResolution;
	graphics_info.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;
	boot_info.graphics_info = &graphics_info;

	// Start Kernel

	typedef unsigned long __attribute__((sysv_abi)) (*KernelEntry)(BOOT_INFO *);
	KernelEntry kernel_start = (KernelEntry) kernel_buffer;

	LOG_INFO(L"BOOTLOADER", L"Starting kernel...\r\n");

	return kernel_start(&boot_info);
}