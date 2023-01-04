#include <cpu/interrupts/interrupts.h>
#include <sys/panik.h>
#include <sys/printk.h>
#include <io/io.h>
#include <dev/8259/pic.h>
#include <dev/tty/kbd/keyboard.h>
#include <dev/tty/gpm/gpm.h>

__attribute__((interrupt)) void PageFault_handler(interrupt_frame *frame) {
        PANIK("Page fault!");

        while (true) {
                asm("hlt");
        }
}

__attribute__((interrupt)) void DoubleFault_handler(interrupt_frame *frame) {
        PANIK("Double fault!");

        while (true) {
                asm("hlt");
        }
}

__attribute__((interrupt)) void GPFault_handler(interrupt_frame *frame) {
        PANIK("General protection fault!");

        while (true) {
                asm("hlt");
        }
}


__attribute__((interrupt)) void KeyboardInt_handler(interrupt_frame *frame) {
        uint8_t scancode = inb(0x60);
        HandleKeyboard(scancode);
        PIC_EndMaster();
}

__attribute__((interrupt)) void MouseInt_handler(interrupt_frame *frame) {
        uint8_t mouse_data = inb(0x60);
        HandlePS2Mouse(mouse_data);
        PIC_EndSlave();
}
