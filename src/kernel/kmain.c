/*
 *  __  __  _                _  __        ___   ___ 
 * |  \/  |(_) __  _ _  ___ | |/ /       / _ \ / __|
 * | |\/| || |/ _|| '_|/ _ \|   <       | (_) |\__ \
 * |_|  |_||_|\__||_|  \___/|_|\_\       \___/ |___/
 *
 * MicroKernel, a simple futiristic Unix-based kernel
 * Copyright (C) 2022-2022 Mutta Filippo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stddef.h>
#include <multiboot.h>
#include <cpuid.h>
#include <sys/panik.h>
#include <mm/memory.h>
#include <cpu/cpu.h>
#include <dev/apic/apic.h>

void kmain(uint64_t multiboot_magic, void *multiboot_data) {
        printk_init();
        printk(" __  __  _                _  __    ___   ___\n"
               "|  \\/  |(_) __  _ _  ___ | |/ /   / _ \\ / __|\n"
               "| |\\/| || |/ _|| '_|/ _ \\|   <   | (_) |\\__ \\\n"
               "|_|  |_||_|\\__||_|  \\___/|_|\\_\\   \\___/ |___/\n");


        printk("%s version %s, build %s %s. Kernel Size %dkb.\n", KNAME, KVER, __DATE__, __TIME__, (V2P(&kernel_end) - V2P(&kernel_start)) / 1024);
        printk("Starting system NOW.\n");
        multiboot_init(multiboot_magic, P2V(multiboot_data));
        printk("Kernel was loaded with command line \"%s\", by <%s>\n", kernel_boot_data.commandline, kernel_boot_data.bootloader);
        memory_init();
        cpu_init();
        apic_init();


        uint64_t *test = P2V(pmm_calloc());
        test[4096] = 1;
        printk("%d\n", test[4096]);

        while (true) {
                asm("hlt");
        }

        PANIK("End of main");
}

