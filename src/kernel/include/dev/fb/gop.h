#pragma once
#include <sys/math.h>
#include <dev/fb/fb.h>
#include <dev/fb/fonts.h>

class GOP{
public:
        GOP(Framebuffer* targetFramebuffer, PSF1_FONT* psf1_Font);
        Point CursorPosition;
        Framebuffer* TargetFramebuffer;
        PSF1_FONT* PSF1_Font;
        unsigned int Colour;
        void Print(const char* str);
        void PutChar(char chr, unsigned int xOff, unsigned int yOff);
};
