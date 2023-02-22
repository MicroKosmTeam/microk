#include <dev/fb/gop.h>
#include <mm/memory.h>
#include <mm/heap.h>

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

        height = target_framebuffer->height;
        width = target_framebuffer->width;
        front_color = 0xffffffff;
        back_color = 0x00000000;
        cursor_position = {0, 0};

	bps = target_framebuffer->bpp * width;

	bufferSize = bps * height;

	buffers[0] = new uint8_t[bufferSize];
	buffers[1] = new uint8_t[bufferSize];

	memset(buffers[0], 0, bufferSize);
	memset(buffers[1], 0, bufferSize);

	currentBuffer = buffers[0];
	nextBuffer = buffers[1];

	bufferChanged = true;
	bufferReady = false;
}

void GOP::BufferSwitch() {
	if(!bufferChanged || !bufferReady) return;

        uint64_t fb_base = (uint64_t)target_framebuffer->address;

	memcpy(currentBuffer, nextBuffer, bufferSize);
	memcpy(fb_base, currentBuffer, bufferSize);

	bufferChanged = false;
}

void GOP::print_clear() {
	bufferReady = false;

        cursor_position.Y = 0;
        cursor_position.X = 0;


        for (int vertical_scanline = 0; vertical_scanline < height; vertical_scanline++) {
                uint64_t pixPtrBase = nextBuffer + (bps * vertical_scanline);
                for (uint32_t *pixPtr = (uint32_t*)pixPtrBase; pixPtr < (uint32_t*)(pixPtrBase+bps); pixPtr++) {
                        *pixPtr = back_color;
                }
        }

	bufferChanged = true;
	bufferReady = true;
}

void GOP::print_scroll() {
	bufferReady = false;

        cursor_position.Y-=16;

	memcpy(nextBuffer, nextBuffer + bps * 16, bufferSize - bps * 16);
	memset(nextBuffer + bufferSize - bps * 16, back_color, bps * 16);
	bufferChanged = true;
	bufferReady = true;
}

uint32_t GOP::get_pixel(uint32_t x, uint32_t y) {
        return *(uint32_t*)((uint64_t)currentBuffer + (x*4) + (y*bps));
}

void GOP::put_pixel(uint32_t x, uint32_t y, uint32_t color) {
	bufferReady = false;
        *(uint32_t*)((uint64_t)nextBuffer + (x*4) + (y*bps)) = color;
	bufferChanged = true;
	bufferReady = true;
}

void GOP::print_char(const char character) {
        if(cursor_position.X + 8 >= width) {
                cursor_position.X = 0;
                cursor_position.Y += 16;
        }

        if(cursor_position.Y + 16*10 >= height) {
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
		bufferReady = false;
                unsigned int* pixPtr = (unsigned int*)nextBuffer;
                char* fontPtr = (char*)psf1_font->glyphBuffer + (character * psf1_font->psf1_Header->charsize);
                for (unsigned long y = cursor_position.Y; y < cursor_position.Y + 16; y++){
                        for (unsigned long x = cursor_position.X; x < cursor_position.X+8; x++){
                                if ((*fontPtr & (0b10000000 >> (x - cursor_position.X))) > 0){
                                        *(unsigned int*)(pixPtr + x + (y * bps /4 )) = front_color;
                                } else {
                                        *(unsigned int*)(pixPtr + x + (y * bps/4)) = back_color;
                                }
                        }

                        fontPtr++;
                }

		bufferChanged = true;
		bufferReady = true;

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
                        cursor_position.X = width;
                        cursor_position.Y -= 16;
                } else {
                        return;
                }
        }

        cursor_position.X -= 8;

	bufferReady = false;
        unsigned int* pixPtr = (unsigned int*)nextBuffer;
        for (unsigned long y = cursor_position.Y; y < cursor_position.Y + 16; y++){
                for (unsigned long x = cursor_position.X; x < cursor_position.X+8; x++){
                        *(unsigned int*)(pixPtr + x + (y * bps / 4)) = back_color;
                }
        }
	bufferChanged = true;
	bufferReady = true;
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
