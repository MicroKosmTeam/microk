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
        void print_char(const char character);
        void print_str(const char* string);
        void print_set_color(const unsigned int foreground, const unsigned int background);

private:
        Point cursor_position;
        Framebuffer *target_framebuffer;
        PSF1_FONT *psf1_font;
        unsigned int back_color;
        unsigned int front_color;
};
