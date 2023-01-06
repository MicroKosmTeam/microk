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

#include <kutil.h>
#include <sys/printk.h>

extern "C" void _start(BootInfo* bootInfo){
        KernelInfo kinfo = kinit(bootInfo);

        printk("Kernel loaded\n");
        printk(" __  __  _                _  __    ___   ___\n");
        printk("|  \\/  |(_) __  _ _  ___ | |/ /   / _ \\ / __|\n");
        printk("| |\\/| || |/ _|| '_|/ _ \\|   <   | (_) |\\__ \\\n");
        printk("|_|  |_||_|\\__||_|  \\___/|_|\\_\\   \\___/ |___/\n");
        printk("The operating system from the future...at your fingertips.\n");
        printk("Kernel is %skb.\n", to_string(kinfo.kernel_size / 1024));
        printk("Free memory: %skb.\n", to_string(GlobalAllocator.GetFreeMem() / 1024));
        printk("Used memory: %skb.\n", to_string(GlobalAllocator.GetUsedMem() / 1024));
        printk("Reserved memory: %skb.\n", to_string(GlobalAllocator.GetReservedMem() / 1024));
        
        print_image();

        GlobalTTY.Activate();

        while (true) {
                asm("hlt");
        }
}
