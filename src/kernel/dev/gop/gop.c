#include <dev/gop/gop.h>

unsigned int xOff, yOff;
Framebuffer *framebuffer;
PSF1_FONT *psf1_font;
const uint64_t gop_color = 0xffffffff;

void gop_init(Framebuffer* new_framebuffer, PSF1_FONT* new_psf1_font) {
        framebuffer = new_framebuffer;
        psf1_font = new_psf1_font;
}


void gop_print_str(char* string) {
        for (size_t i = 0; 1; i++) {
                char character = (uint8_t) string[i];

                if (character == '\0') {
                        return;
                }

                gop_print_char(character);
        }
}

void gop_print_char(char character) {
        unsigned int* pixPtr = (unsigned int*)framebuffer->BaseAddress;
        char* fontPtr = psf1_font->glyphBuffer + (character * psf1_font->psf1_Header->charsize);
        for (unsigned long y = yOff; y < yOff + 16; y++){
                for (unsigned long x = xOff; x < xOff+8; x++){
                        if ((*fontPtr & (0b10000000 >> (x - xOff))) > 0){
                                *(unsigned int*)(pixPtr + x + (y * framebuffer->PixelsPerScanLine)) = gop_color;
                        }
                }

                fontPtr++;
        }

        xOff+=8;
        yOff+=0;
}
