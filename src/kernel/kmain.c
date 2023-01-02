#include <stdio.h>
#include <sys/panik.h>
#include <dev/serial/serial.h>
#include <mm/efimem.h>

#include "kmain.h"

void kmain() {
        printk(" __  __  _                _  __    ___   ___\n"
               "|  \\/  |(_) __  _ _  ___ | |/ /   / _ \\ / __|\n"
               "| |\\/| || |/ _|| '_|/ _ \\|   <   | (_) |\\__ \\\n"
               "|_|  |_||_|\\__||_|  \\___/|_|\\_\\   \\___/ |___/\n");

        printk("%s version %s, build %s %s.\n", KNAME, KVER, __DATE__, __TIME__);
        printk("Starting system NOW.\n");

        while(true){
                asm ("hlt");
        }

        asm("hlt");

        PANIK("End of main");
}

void efi_main(BootInfo* boot_info) {
        serial_init(COM1);
        serial_print_str("Serial loaded.\n\r");

        gop_init(boot_info->framebuffer, boot_info->psf1_Font);
        gop_print_clear();
        serial_print_str("Gop Loaded\n");

        uint64_t mMapEntries = boot_info->mMapSize / boot_info->mMapDescSize;

        printk("Map Entries: %d\n", mMapEntries);

        for (int i = 0; i < mMapEntries; i++){
                EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)boot_info->mMap + (i * boot_info->mMapDescSize));
                printk("%s %dkb\n", EFI_MEMORY_TYPE_STRINGS[desc->type], desc->numPages * 4096 / 1024);
        }

        serial_print_str("Jumping to kmain.\n\r");
        kmain();
}
