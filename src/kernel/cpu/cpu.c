#include <cpu.h>
#include <interrupts.h>
#include <stdio.h>

void cpu_init() {
        printk("Loading interrupts...\n");
        interrupt_init();
        printk("Interrupts loaded!\n");
}
