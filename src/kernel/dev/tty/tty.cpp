#include <dev/tty/tty.h>
#include <dev/fb/gop.h>
#include <sys/printk.h>
#include <sys/panik.h>
#include <mm/memory.h>
#include <sys/module.h>
#include <mm/string.h>
#include <mm/pageframe.h>
#include <sys/cstr.h>
#include <dev/timer/pit/pit.h>
#include <fs/fs.h>
#include <kutil.h>

TTY GlobalTTY;

TTY::TTY() {
        is_active = false;
}

TTY::~TTY() {
}

bool TTY::Activate() {
        printk("Activating TTY...\n\n");
        user_mask = (char*)malloc(512);
        
        PrintPrompt();
        // We activate it at the end
        is_active = true;
}

void TTY::Deactivate() {
        printk("Dectivating TTY...\n");
        // We deactivate it immediatly
        is_active = false;

        free(user_mask);
}

void TTY::PrintPrompt() {
        printk("(kernel mode) $ ");
}

void TTY::ElaborateCommand() {
        printk("\n");

        char *ptr = strtok(user_mask, " ");

        if (strcmp(ptr, "help") == 0) {
                printk("Available commands\n"
                       "cls\n"
                       "uname\n"
                       "module\n"
                       "panik\n"
                       "mem\n"
                       "time\n"
                       "rdinit\n"
                       "image\n");
        } else if (strcmp(ptr, "cls") == 0 || strcmp(ptr, "clear") == 0 || strcmp(ptr, "clean") == 0) {
                GlobalRenderer.print_clear();
        } else if (strcmp(ptr, "uname") == 0) {
                printk("MicroK Alpha.\n");
        } else if (strcmp(ptr, "module") == 0) {
                ptr = strtok(NULL, " ");
                if (ptr != NULL) {
                        if(GlobalModuleManager.FindModule(ptr)) {
                                printk("Module found! Loading...\n");
                                if(GlobalModuleManager.LoadModule(ptr)) {
                                        printk("Module loaded successfully.\n");
                                } else {
                                        printk("Loading module failed!\n");
                                }
                        } else {
                                printk("No module found: %s\n", ptr);
                        }
                } else {
                        printk("Insert a module name.\n");
                }
        } else if (strcmp(ptr, "panik") == 0) {
                PANIK("User induced panic.");
        } else if (strcmp(ptr, "mem") == 0) {
                printk("Free memory: %s.\n", to_string(GlobalAllocator.GetFreeMem()));
                printk("Used memory: %s.\n", to_string(GlobalAllocator.GetUsedMem()));
                printk("Reserved memory: %s.\n", to_string(GlobalAllocator.GetReservedMem()));
        } else if (strcmp(ptr, "time") == 0) {
                printk("Current tick: %d.\n", (uint64_t)PIT::TimeSinceBoot);
        } else if (strcmp(ptr, "image") == 0) {
                print_image(1);
        } else if (strcmp(ptr, "lsblk") == 0) {
                GlobalFSManager.ListDrives();
        } else if (strcmp(ptr, "rdinit") == 0) {
                rdinit();
        } else {
                printk("Unknown command: %s\n", ptr);
        }

        printk("\n");

}

void TTY::SendChar(char ch) {
        if (!is_active) return;

        static uint8_t curr_char;

        switch (ch) {
                case '\b':
                        if (curr_char > 0) {
                                user_mask[--curr_char] = NULL;
                                GlobalRenderer.clear_char();
                        }
                        break;
                case '\r':
                        break;
                case '\n':
                        is_active = false;
                        ElaborateCommand();
                        PrintPrompt();
                        memset(user_mask, 0, 512);
                        curr_char = 0;
                        is_active = true;
                        break;
                default:
                        if (curr_char < 512) {
                                user_mask[curr_char++] = ch;
                                GlobalRenderer.print_char(ch);
                        }
                        break;
        }
}


