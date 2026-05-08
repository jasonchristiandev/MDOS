#pragma once
#include <efi.h>

typedef struct {
	void *framebuffer_base;
	UINTN framebuffer_size;
	UINT32 width;
	UINT32 height;
	UINT32 pixels_per_scanline;
} BOOT_INFO;