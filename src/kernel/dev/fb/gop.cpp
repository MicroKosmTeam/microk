#include <dev/fb/gop.h>

GOP::GOP() {
        target_framebuffer = NULL;
        psf1_font = NULL;
        
        front_color = 0xffffffff;
        back_color = 0x00000000;
        cursor_position = {0, 0};
}

GOP::GOP(Framebuffer *new_target_framebuffer, PSF1_FONT *new_psf1_font) {
        target_framebuffer = new_target_framebuffer;
        psf1_font = new_psf1_font;
        
        front_color = 0xffffffff;
        back_color = 0x00000000;
        cursor_position = {0, 0};
}

void GOP::print_clear() {

}

void GOP::print_char(const char character) {
        unsigned int* pixPtr = (unsigned int*)target_framebuffer->BaseAddress;
        char* fontPtr = (char*)psf1_font->glyphBuffer + (character * psf1_font->psf1_Header->charsize);
        for (unsigned long y = cursor_position.Y; y < cursor_position.Y + 16; y++){
                for (unsigned long x = cursor_position.X; x < cursor_position.X+8; x++){
                        if ((*fontPtr & (0b10000000 >> (x - cursor_position.X))) > 0){
                                *(unsigned int*)(pixPtr + x + (y * target_framebuffer->PixelsPerScanLine)) = front_color;
                        } else {
                                *(unsigned int*)(pixPtr + x + (y * target_framebuffer->PixelsPerScanLine)) = back_color;
                        }
                }

                fontPtr++;
        }
}

void GOP::print_str(const char* string) {
        char *character = (char*)string;
        while(*character != 0) {
                switch (*character) {
                        case '\n':
                                cursor_position.X=0;
                                cursor_position.Y+=16;
                                break;
                        case '\r':
                                cursor_position.X=0;
                                break;
                        default:
                                print_char(*character);
                                cursor_position.X+=8;
                }



                if(cursor_position.X + 8 > target_framebuffer->Width) {
                        cursor_position.X = 0;
                        cursor_position.Y += 16;
                }

                character++;
        }
}

void GOP::print_set_color(const unsigned int foreground, const unsigned int background) {
        front_color = foreground;
        back_color = background;
}
