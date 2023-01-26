#include <sys/panik.h>
#include <sys/printk.h>
#include <cdefs.h>

void panik(const char *message, const char *file, const char *function, unsigned int line) {
        asm volatile ("cli"); // We don't want interrupts while we are panicking

        #ifdef KCONSOLE_GOP
        GlobalRenderer.print_set_color(0xff0000ff, 0x00000000);
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
