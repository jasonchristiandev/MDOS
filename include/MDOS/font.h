#pragma once

#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04

typedef struct {
	unsigned char magic[2];
	unsigned char mode;
	unsigned char char_size;
} PSF1_HEADER;

typedef struct {
	PSF1_HEADER *header;
	void *glyph_buffer;
} PSF1_FONT;

#define PSF2_MAGIC0 0x72
#define PSF2_MAGIC1 0xb5
#define PSF2_MAGIC2 0x4a
#define PSF2_MAGIC3 0x86

typedef struct {
	unsigned char magic[4];
	unsigned int version;
	unsigned int header_size;
	unsigned int flags;
	unsigned int num_glyph;
	unsigned int bytes_per_glyph;
	unsigned int height;
	unsigned int width;
} PSF2_HEADER;