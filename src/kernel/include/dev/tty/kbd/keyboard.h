#pragma once
#include <stdint.h>

void HandleKeyboard(uint8_t scancode);

namespace QWERTYKeyboard {
        #define LSHIFT 0x2A
        #define RSHIFT 0x36
        #define RETURN 0x1C
        #define BACKSP 0x0E
        #define SPCBAR 0x39

        extern const char ASCIITable[];
        char Translate(uint8_t scancode, bool uppercase);
}
