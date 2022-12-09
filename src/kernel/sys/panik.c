#include <sys/panik.h>

void panik(const char *message, const char *file, const char *function, unsigned int line) {
        asm volatile ("cli");
#ifdef KCONSOLE_SERIAL
        // Do shit
#endif
#ifdef KCONSOLE_VGA
        vga_print_set_color(PRINT_COLOR_RED, PRINT_COLOR_WHITE);
#endif
#ifdef KCONSOLE_VBE
        vbe_print_set_color(PRINT_COLOR_RED, PRINT_COLOR_WHITE);
#endif
#ifdef KCONSOLE_GOP
        gop_print_set_color(PRINT_COLOR_RED, PRINT_COLOR_WHITE);
#endif
        printk("\n\n!!PANIK!!\n\n");
        printk("%s version %s, build %s %s\n", KNAME, KVER, __DATE__, __TIME__);
        printk("%s in function %s at line %d\n", file, function, line);
        printk("Cause: %s\n", message);
        printk("[Hanging now...]\n");
        while (true) {
                asm volatile ("hlt");
        }

}
