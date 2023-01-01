#pragma once
#include <stddef.h>
#include <stdint.h>

#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04

typedef struct {
	unsigned char magic[2];
	unsigned char mode;
	unsigned char charsize;
} PSF1_HEADER;

typedef struct {
	PSF1_HEADER* psf1_Header;
	void* glyphBuffer;
} PSF1_FONT;

typedef struct {
	void* BaseAddress;
	size_t BufferSize;
	unsigned int Width;
	unsigned int Height;
	unsigned int PixelsPerScanLine;
} Framebuffer;

void gop_init(Framebuffer* framebuffer, PSF1_FONT* psf1_font);
void gop_print_clear();
void gop_print_char(char character);
void gop_print_str(char* string);
void gop_print_set_color(uint8_t foreground, uint8_t background);
