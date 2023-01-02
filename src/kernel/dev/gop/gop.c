#include <dev/gop/gop.h>

unsigned int xOff, yOff;
Framebuffer *framebuffer;
PSF1_FONT *psf1_font;
static uint64_t text_color = 0xffffffff;
static uint64_t back_color = 0x00000000;

void gop_init(Framebuffer* new_framebuffer, PSF1_FONT* new_psf1_font) {
        framebuffer = new_framebuffer;
        psf1_font = new_psf1_font;
}

void gop_print_set_color(uint64_t foreground, uint64_t background) {
        text_color = foreground;
        back_color = background;
}

static void clean_line(size_t row) {
        unsigned int* pixPtr = (unsigned int*)framebuffer->BaseAddress;
        
        for (unsigned long x = xOff; x < framebuffer->Width; x++){
                for (int i = 0; i < 16; i++) {
                        *(unsigned int*)(pixPtr + x + ((row+i) * framebuffer->PixelsPerScanLine)) = back_color;
                }
        }

}

void gop_print_clear() {
        for (unsigned long y = 0; y < framebuffer->Height; y+=16) {
                clean_line(y);
        }
        yOff = 0;
        xOff = 0;
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

static void print_newline() {
        if (yOff < framebuffer->Height - 16) {
                yOff+=16;
        } else {
                // Probably we need to fix this
                //gop_print_clear();
        }

        xOff = 0;
}

void gop_print_char(char character) {
        if (character == '\n') {
                print_newline();
                return;
        } else if (character == '\r') {
                xOff = 0;
        } else if (xOff + 8 > framebuffer->Width) {
                print_newline();
        }

        unsigned int* pixPtr = (unsigned int*)framebuffer->BaseAddress;
        char* fontPtr = psf1_font->glyphBuffer + (character * psf1_font->psf1_Header->charsize);
        for (unsigned long y = yOff; y < yOff + 16; y++){
                for (unsigned long x = xOff; x < xOff+8; x++){
                        if ((*fontPtr & (0b10000000 >> (x - xOff))) > 0){
                                *(unsigned int*)(pixPtr + x + (y * framebuffer->PixelsPerScanLine)) = text_color;
                        } else {
                                *(unsigned int*)(pixPtr + x + (y * framebuffer->PixelsPerScanLine)) = back_color;
                        }
                }

                fontPtr++;
        }

        xOff+=8;
}
