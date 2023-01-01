#include <stdio.h>
#include <multiboot.h>
#include <cpuid.h>
#include <sys/panik.h>
#include <mm/memory.h>
#include <cpu/cpu.h>
#include <dev/apic/apic.h>
#include <dev/acpi/acpi.h>
#include <dev/serial/serial.h>

#include "kmain.h"

void kmain() {
        printk(" __  __  _                _  __    ___   ___\n"
               "|  \\/  |(_) __  _ _  ___ | |/ /   / _ \\ / __|\n"
               "| |\\/| || |/ _|| '_|/ _ \\|   <   | (_) |\\__ \\\n"
               "|_|  |_||_|\\__||_|  \\___/|_|\\_\\   \\___/ |___/\n");

        printk("%s version %s, build %s %s.\nKernel At 0x%x - 0x%x. Size %dkb.\n", KNAME, KVER, __DATE__, __TIME__, V2P(&kernel_start), V2P(&kernel_end), (V2P(&kernel_end) - V2P(&kernel_start)) / 1024);
        printk("Starting system NOW.\n");

        printk("Line lenght test: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaare we done yet?\n");

        while(true){
                asm ("hlt");
        }

        cpu_init();

        memory_init();
        acpi_init();
        apic_init();

        printk("Boot process complete!\n");

        uint64_t *test_a;
        uint64_t *test_b;
        test_a = P2V(pmm_calloc());
        printk("Variable A is at: 0x%x\n", V2P(&test_a));
        test_a[0] = 69;
        pmm_free(V2P(&test_a));

        printk("Done allocating...\n");

        asm("hlt");

        PANIK("End of main");
}

void efi_main(BootInfo* boot_info) {
        serial_init(COM1);
        serial_print_str("Serial loaded.\n\r");

        gop_init(boot_info->framebuffer, boot_info->psf1_Font);
        gop_print_clear();
        gop_print_str("Gop Loaded\n");

        asm("sti");

        serial_print_str("Jumping to kmain.\n\r");
        kmain();
}

void multiboot_main(uint64_t multiboot_magic, void *multiboot_data) {
        printk_init();
        multiboot_init(multiboot_magic, P2V(multiboot_data));
        printk("Kernel was loaded with command line \"%s\", by <%s>\n", kernel_boot_data.commandline, kernel_boot_data.bootloader);

        kmain(); // KMAIN
}
