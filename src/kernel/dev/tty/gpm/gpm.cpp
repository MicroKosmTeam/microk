#include <dev/tty/gpm/gpm.h>
#include <io/io.h>
#include <sys/printk.h>

uint8_t mouse_pointer[] = {
        0b11111111, 0b11000000,
        0b11111111, 0b10000000,
        0b11111110, 0b00000000,
        0b11111100, 0b00000000,
        0b11111000, 0b00000000,
        0b11110000, 0b00000000,
        0b11100000, 0b00000000,
        0b11000000, 0b00000000,
        0b11000000, 0b00000000,
        0b10000000, 0b00000000,
        0b10000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
};

static void MouseWait() {
        uint64_t timeout = 100000;
        while(timeout--) if((inb(0x64) & 0b10) == 0) return;
}

static void MouseWaitInput() {
        uint64_t timeout = 100000;
        while(timeout--) if(inb(0x64) & 0b1) return;
}

static void MouseWrite(uint8_t value) {
        MouseWait();
        outb(0x64, 0xD4);
        MouseWait();
        outb(0x60, value);
}

static uint8_t MouseRead() {
        MouseWaitInput();
        return inb(0x60);
}

uint8_t mouse_cycle = 0;
uint8_t mouse_packet[4];
bool mouse_packet_ready = false;
Point mouse_position;
Point mouse_position_old;
void HandlePS2Mouse(uint8_t data) {
        switch (mouse_cycle) {
                case 0:
                        if (mouse_packet_ready) break;
                        if ((data & 0b00001000) == 0) break;
                        mouse_packet[0] = data;
                        mouse_cycle++;
                        break;  
                case 1:
                        if (mouse_packet_ready) break;
                        mouse_packet[1] = data;
                        mouse_cycle++;
                        break;
                case 2:
                        if (mouse_packet_ready) break;
                        mouse_packet[2] = data;
                        mouse_packet_ready = true;
                        mouse_cycle = 0;
                        break;
                // Ignored:
                //case 3:
                //        break;
                default:
                        mouse_cycle = 0;
                        break;
        }
        
}

void InitPS2Mouse() {
        outb(0x64, 0xA8); // Enable the auxiliarry device mouse
        MouseWait();

        outb(0x64, 0x20); // Tell send command to mouse
        MouseWaitInput();

        uint8_t status = inb(0x60);
        status |= 0b10;
        MouseWait();
        outb(0x64, 0x60);
        MouseWait();
        outb(0x60, status); // Setting "compaq" status byte
        
        MouseWrite(0xF6); // Default settings
        MouseRead();

        MouseWrite(0xF4);
        MouseRead();
}

void ProcessMousePacket() {
        if (!mouse_packet_ready) return;
        else mouse_packet_ready = false;


        bool xnegative, ynegative, xoverflow, yoverflow;

        if (mouse_packet[0] & PS2XSIGN) xnegative = true;
        else xnegative = false;

        if (mouse_packet[0] & PS2YSIGN) ynegative = true;
        else ynegative = false;

        if (mouse_packet[0] & PS2XOVER) xoverflow = true;
        else xoverflow = false;

        if (mouse_packet[0] & PS2YOVER) yoverflow = true;
        else yoverflow = false;


        if (!xnegative) {
                mouse_position.X += mouse_packet[1];
                if(xoverflow) {
                        mouse_position.X += 255;
                }
        } else {
                mouse_packet[1] = 256 - mouse_packet[1];
                mouse_position.X -= mouse_packet[1];
                if(xoverflow) {
                        mouse_position.X -= 255;
                }
        }

        if (!ynegative) {
                mouse_position.Y -= mouse_packet[2];
                if(yoverflow) {
                        mouse_position.Y -= 255;
                }
        } else {
                mouse_packet[2] = 256 - mouse_packet[2];
                mouse_position.Y += mouse_packet[2];
                if(yoverflow) {
                        mouse_position.Y += 255;
                }
        }

        if (mouse_position.X < 0) mouse_position.X = 0;
        if (mouse_position.X > GlobalRenderer.width - 1) mouse_position.X = GlobalRenderer.width - 1;
        if (mouse_position.Y < 0) mouse_position.Y = 0;
        if (mouse_position.Y > GlobalRenderer.height - 1) mouse_position.Y = GlobalRenderer.height - 1;

        GlobalRenderer.clear_mouse_cursor(mouse_pointer, mouse_position_old);
        GlobalRenderer.draw_mouse_cursor(mouse_pointer, mouse_position, 0xffffffff);
        
        if (mouse_packet[0] & PS2LFBUT) {
                GlobalRenderer.print_char_pos('l', mouse_position);
        }

        if (mouse_packet[0] & PS2MDBUT) {
                GlobalRenderer.print_char_pos('m', mouse_position);
        }

        if (mouse_packet[0] & PS2RTBUT) {
                GlobalRenderer.print_char_pos('r', mouse_position);
        }

        mouse_packet_ready = false;
        mouse_position_old = mouse_position;
}
