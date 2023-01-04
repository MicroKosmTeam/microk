#include <dev/tty/kbd/keyboard.h>
#include <sys/printk.h>
#include <dev/fb/gop.h>

bool lshift_pressed;
bool rshift_pressed;

void HandleKeyboard(uint8_t scancode) {
        switch (scancode) {
                case LSHIFT:
                        lshift_pressed = true;
                        break;
                case LSHIFT + 0x80:
                        lshift_pressed = false;
                        break;
                case RSHIFT:
                        rshift_pressed = true;
                        break;
                case RSHIFT + 0x80:
                        rshift_pressed = false;
                        break;
                case RETURN:
                        GlobalRenderer.print_char('\n');
                        break;
                case BACKSP:
                        GlobalRenderer.clear_char();
                        break;
                case SPCBAR:
                        GlobalRenderer.print_char(' ');
                        break;
                default:
                        char character = QWERTYKeyboard::Translate(scancode, lshift_pressed | rshift_pressed);

                        if(character != 0)
                                GlobalRenderer.print_char(character);
                        break;
        }



}

namespace QWERTYKeyboard {
        const char ASCIITable[] = {
                 0 ,  0 , '1', '2',
                '3', '4', '5', '6',
                '7', '8', '9', '0',
                '-', '=',  0 ,  0 ,
                'q', 'w', 'e', 'r',
                't', 'y', 'u', 'i',
                'o', 'p', '[', ']',
                 0 ,  0 , 'a', 's',
                'd', 'f', 'g', 'h',
                'j', 'k', 'l', ';',
                '\'','`',  0 , '\\',
                'z', 'x', 'c', 'v',
                'b', 'n', 'm', ',',
                '.', '/',  0 , '*',
                 0 , ' '
        };

        char Translate(uint8_t scancode, bool uppercase){
                if (scancode > 58) return 0;

                if (uppercase) return ASCIITable[scancode] - 32;
                else return ASCIITable[scancode];
        }
}

