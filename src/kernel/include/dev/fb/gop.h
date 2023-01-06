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
        void print_clear();
        void clear_char();
        void print_char(const char character);
        void print_char_pos(const char character, Point pos);
        void print_str(const char* string);
        void print_set_color(const unsigned int foreground, const unsigned int background);
        void print_scroll();

        uint32_t get_pixel(uint32_t x, uint32_t y);
        void put_pixel(uint32_t x, uint32_t y, uint32_t color);

        uint32_t MouseCursorBuffer[16*16];
        uint32_t MouseCursorBufferAfter[16*16];
        void clear_mouse_cursor(uint8_t *mouse_pointer, Point pos);
        void draw_mouse_cursor(uint8_t *mouse_pointer, Point pos, uint32_t color);

        unsigned int height, width;
        bool mouse_drawn;
private:
        Framebuffer *target_framebuffer;
        Point cursor_position;
        PSF1_FONT *psf1_font;
        unsigned int back_color;
        unsigned int front_color;
};
