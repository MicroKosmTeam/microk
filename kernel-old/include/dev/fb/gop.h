#pragma once
#include <stdint.h>
#include <stddef.h>
#include <sys/math.h>
#include <dev/fb/fb.h>
#include <dev/fb/fonts.h>

class GOP{
public:
        GOP();
        GOP(Framebuffer *new_target_framebuffer, PSF1_FONT *new_psf1_font);
	void BufferSwitch();
        void print_clear();
        void clear_char();
        void print_char(const char character);
        void print_char_pos(const char character, Point pos);
        void print_str(const char* string);
        void print_set_color(const unsigned int foreground, const unsigned int background);
        void print_scroll();

        uint32_t get_pixel(uint32_t x, uint32_t y);
        void put_pixel(uint32_t x, uint32_t y, uint32_t color);

        unsigned int height, width;
private:
        Framebuffer *target_framebuffer;
        Point cursor_position;
        PSF1_FONT *psf1_font;

	uint8_t *buffers[2];
	uint8_t *currentBuffer;
	uint8_t *nextBuffer;
	bool bufferReady;
	bool bufferChanged;
	size_t bufferSize;

	uint64_t bps;

        unsigned int back_color;
        unsigned int front_color;
};

