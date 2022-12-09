#include <dev/apic/apic.h>
#include <dev/8259/pic.h>

void apic_init() {
        pic_disable();
}
