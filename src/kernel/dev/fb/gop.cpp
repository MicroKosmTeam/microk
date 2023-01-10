#include <dev/fb/gop.h>
#include <mm/memory.h>

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
        
        height = target_framebuffer->Height;
        width = target_framebuffer->Width;
        front_color = 0xffffffff;
        back_color = 0x00000000;
        cursor_position = {0, 0};
}

void GOP::print_clear() {
        cursor_position.Y = 0;
        cursor_position.X = 0;

        uint64_t fb_base = (uint64_t)target_framebuffer->BaseAddress;
        uint64_t bps = target_framebuffer->PixelsPerScanLine * 4;
        uint64_t fb_height = target_framebuffer->Height;

        for (int vertical_scanline = 0; vertical_scanline < fb_height; vertical_scanline++) {
                uint64_t pixPtrBase = fb_base + (bps * vertical_scanline);
                for (uint32_t *pixPtr =(uint32_t*)pixPtrBase; pixPtr < (uint32_t*)(pixPtrBase+bps); pixPtr++) {
                        *pixPtr = back_color;
                }
        }
}

void GOP::print_scroll() {
        cursor_position.Y-=16;
        uint64_t fb_base = (uint64_t)target_framebuffer->BaseAddress;
        uint64_t bps = target_framebuffer->PixelsPerScanLine * 4;
        uint64_t fb_height = target_framebuffer->Height;

        for (int vertical_scanline = 0; vertical_scanline < fb_height - 16; vertical_scanline++) {
                uint64_t pixPtrBase = fb_base + (bps * vertical_scanline);
                uint64_t pixPtrBaseNext = fb_base + (bps * (vertical_scanline+16));
        
                uint32_t *pixPtr =(uint32_t*)pixPtrBase;
                uint32_t *pixNextPtr =(uint32_t*)pixPtrBaseNext;
                for (; pixPtr < (uint32_t*)(pixPtrBase+bps); pixPtr++, pixNextPtr++) {
                        *pixPtr = *pixNextPtr;
                }
        }
}

uint32_t GOP::get_pixel(uint32_t x, uint32_t y) {
        return *(uint32_t*)((uint64_t)target_framebuffer->BaseAddress + (x*4) + (y*target_framebuffer->PixelsPerScanLine*4));
}

void GOP::put_pixel(uint32_t x, uint32_t y, uint32_t color) {
        *(uint32_t*)((uint64_t)target_framebuffer->BaseAddress + (x*4) + (y*target_framebuffer->PixelsPerScanLine*4)) = color;
}

void GOP::clear_mouse_cursor(uint8_t *mouse_pointer, Point pos) {
        if (!mouse_drawn) return;
        int xmax = 16;
        int ymax = 16;
        int diffx = width - pos.X;
        int diffy = height - pos.Y;

        if(diffx < 16) xmax = diffx;
        if(diffy < 16) ymax = diffy;

        for (int y = 0; y < ymax; y++) {
                for (int x = 0; x < xmax; x++) {
                        int bit = y *16 + x;
                        int byte = bit / 8;
                        if((mouse_pointer[byte] & (0b10000000 >> (x % 8)))) {
                                if (get_pixel(pos.X +x, pos.Y +y) == MouseCursorBufferAfter[x+y*16]) {
                                        put_pixel(pos.X + x, pos.Y + y, MouseCursorBuffer[x+y*16]);
                                }

                        }
                }
        }
}

void GOP::draw_mouse_cursor(uint8_t *mouse_pointer, Point pos, uint32_t color) {
        int xmax = 16;
        int ymax = 16;
        int diffx = width - pos.X;
        int diffy = height - pos.Y;

        if(diffx < 16) xmax = diffx;
        if(diffy < 16) ymax = diffy;

        for (int y = 0; y < ymax; y++) {
                for (int x = 0; x < xmax; x++) {
                        int bit = y *16 + x;
                        int byte = bit / 8;
                        if((mouse_pointer[byte] & (0b10000000 >> (x % 8)))) {
                                MouseCursorBuffer[x+y*16] = get_pixel(pos.X + x, pos.Y + y);
                                put_pixel(pos.X + x, pos.Y + y, color);
                                MouseCursorBufferAfter[x+y*16] = get_pixel(pos.X + x, pos.Y + y);
                        }
                }
        }

        mouse_drawn = true;

}

void GOP::print_char(const char character) {
        if(cursor_position.X + 8 >= target_framebuffer->Width) {
                cursor_position.X = 0;
                cursor_position.Y += 16;
        }

        if(cursor_position.Y + 16*10 >= target_framebuffer->Height) {
                print_scroll();
        }

        switch (character) {
        case '\n':
                cursor_position.X=0;
                cursor_position.Y+=16;
                break;
        case '\r':
                cursor_position.X=0;
                break;
        default:
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

                cursor_position.X += 8;
        }
}

void GOP::print_char_pos(const char character, Point pos) {
        Point oldpos = cursor_position;
        cursor_position = pos;
        print_char(character);
        cursor_position = oldpos;
}

void GOP::clear_char() {
        if (cursor_position.X == 0) {
                if (cursor_position.Y >=16) {
                        cursor_position.X = target_framebuffer->Width;
                        cursor_position.Y -= 16;
                } else {
                        return;
                }
        } 

        cursor_position.X -= 8;

        unsigned int* pixPtr = (unsigned int*)target_framebuffer->BaseAddress;
        for (unsigned long y = cursor_position.Y; y < cursor_position.Y + 16; y++){
                for (unsigned long x = cursor_position.X; x < cursor_position.X+8; x++){
                        *(unsigned int*)(pixPtr + x + (y * target_framebuffer->PixelsPerScanLine)) = back_color;
                }
        }
}

void GOP::print_str(const char* string) {
        char *character = (char*)string;
        while(*character != 0) {

                
                print_char(*character); 

                character++;
        }
}

void GOP::print_set_color(const unsigned int foreground, const unsigned int background) {
        front_color = foreground;
        back_color = background;
}
