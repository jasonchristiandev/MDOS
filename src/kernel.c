#include "MDOS/bootinfo.h"
#include "MDOS/font.h"

extern char _binary_font_psf_start[];

UINT32 term_x = 0;
UINT32 term_y = 0;
unsigned char char_size;
void *glyph_buffer;

void draw_char(BOOT_INFO *info, char c, UINT32 start_x, UINT32 start_y, UINT32 color);
void term_print(BOOT_INFO *info, char *msg);

void __attribute__((sysv_abi)) _start(BOOT_INFO *info) {
	__asm__("and $0xfffffffffffffff0, %rsp");

	UINT32 *buffer = (UINT32 *) info->framebuffer_base;

	for (UINT32 i = 0; i < info->width * info->height; i++) {
		buffer[i] = 0x00000000;
	}

	PSF1_HEADER *h1 = (PSF1_HEADER *) _binary_font_psf_start;
	PSF2_HEADER *h2 = (PSF2_HEADER *) _binary_font_psf_start;

	if (h1->magic[0] == PSF1_MAGIC0 && h1->magic[1] == PSF1_MAGIC1) {
		char_size = h1->char_size;
		glyph_buffer = (void *) (_binary_font_psf_start + sizeof(PSF1_HEADER));
	} else if (h2->magic[0] == PSF2_MAGIC0 && h2->magic[1] == PSF2_MAGIC1) {
		char_size = h2->bytes_per_glyph;
		glyph_buffer = (void *) (_binary_font_psf_start + h2->header_size);
	} else {
		for (UINT32 i = 0; i < info->width * info->height; i++) {
			buffer[i] = 0x00FF0000;
		}
		while (1) {
			asm("hlt");
		}
	}

	draw_char(info, 'M', 16, 16, 0xFFFFFFFF);
	draw_char(info, 'D', 24, 16, 0xFFFFFFFF);
	draw_char(info, 'O', 32, 16, 0xFFFFFFFF);
	draw_char(info, 'S', 40, 16, 0xFFFFFFFF);

	while (1) {
		asm("hlt");
	}
}

void draw_char(BOOT_INFO *info, char c, UINT32 start_x, UINT32 start_y, UINT32 color) {
	UINT32 *buffer = (UINT32 *) info->framebuffer_base;

	char *font_ptr = (char *) glyph_buffer + (c * char_size);

	for (UINT32 y = 0; y < char_size; y++) {
		for (UINT32 x = 0; x < 8; x++) {
			if ((*font_ptr >> (7 - x)) & 1) {
				UINT32 index = ((start_y + y) * info->pixels_per_scanline) + (start_x + x);
				buffer[index] = color;
			}
		}
		font_ptr++;
	}
}

void term_print(BOOT_INFO *info, char *msg) {
	//DrawChar(, char c, UINT32 start_x, UINT32 start_y, UINT32 color)
}