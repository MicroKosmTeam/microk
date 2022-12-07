#include <stdio.h>
#include <stddef.h>
#include <multiboot.h>
#include <panik.h>
#include <memory.h>
#include <cpu.h>

/*
  __  __  _                _  __        ___   ___ 
 |  \/  |(_) __  _ _  ___ | |/ /       / _ \ / __|
 | |\/| || |/ _|| '_|/ _ \|   <       | (_) |\__ \
 |_|  |_||_|\__||_|  \___/|_|\_\       \___/ |___/

 */

void kmain(uint64_t multiboot_magic, void *multiboot_data) {
        printk_init();
        printk(" __  __  _                _  __    ___   ___\n"
               "|  \\/  |(_) __  _ _  ___ | |/ /   / _ \\ / __|\n"
               "| |\\/| || |/ _|| '_|/ _ \\|   <   | (_) |\\__ \\\n"
               "|_|  |_||_|\\__||_|  \\___/|_|\\_\\   \\___/ |___/\n");


        printk("%s version %s, build %s %s\n", KNAME, KVER, __DATE__, __TIME__);
        printk("Starting system NOW.\n");
        printk("kernel_start is 0x%x\n", V2P(&kernel_start));
        printk("kernel_end is 0x%x\n", V2P(&kernel_end));
        multiboot_init(multiboot_magic, P2V(multiboot_data));
        printk("Command line: %s\nBootloader: %s\n", kernel_boot_data.commandline, kernel_boot_data.bootloader);

        memory_init();
        cpu_init();

        while (true);

        PANIK("End of main");
}
