#include <arch/x86_64/interrupts/interrupts.h>
#include <sys/panik.h>

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
