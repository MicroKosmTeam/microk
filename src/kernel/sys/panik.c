#include <sys/panik.h>

void panik(const char *message, const char *file, const char *function, unsigned int line) {
        asm volatile ("cli"); // We don't want interrupts while we are panicking
#ifdef KCONSOLE_SERIAL
        // Nothing to do for now
#endif
#ifdef KCONSOLE_VGA
        vga_print_set_color(PRINT_COLOR_RED, PRINT_COLOR_WHITE);
#endif
#ifdef KCONSOLE_VBE
        vbe_print_set_color(PRINT_COLOR_RED, PRINT_COLOR_WHITE);
#endif
#ifdef KCONSOLE_GOP
        //gop_print_set_color(PRINT_COLOR_RED, PRINT_COLOR_WHITE);
#endif
        // Printing the panic message
        printk("\n\n!! PANIK!! \n");
        printk("Irrecoverable error in the kernel.\n\n");
        printk("%s version %s, build %s %s\n", KNAME, KVER, __DATE__, __TIME__);
        printk("%s in function %s at line %d\n", file, function, line);
        printk("Cause: %s\n", message);
        printk("[Hanging now...]\n");

        while (true) {
                // Halting forever
                asm volatile ("hlt");
        }

}
