#pragma once
#include <efi.h>

typedef struct {
	void *framebuffer_base;
	UINTN framebuffer_size;
	UINT32 width;
	UINT32 height;
	UINT32 pixels_per_scanline;
} GRAPHICS_INFO;

typedef struct {
	EFI_SYSTEM_TABLE *system_table;
	GRAPHICS_INFO *graphics_info;
} BOOT_INFO;