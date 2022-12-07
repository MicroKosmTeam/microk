#include <panik.h>

void panik(const char *message, const char *file, const char *function, unsigned int line) {
        asm volatile ("cli");
        print_set_color(PRINT_COLOR_RED, PRINT_COLOR_WHITE);
        printk("\n\n!!PANIK!!\n\n");
        printk("%s version %s, build %s %s\n", KNAME, KVER, __DATE__, __TIME__);
        printk("%s in function %s at line %d\n", file, function, line);
        printk("Cause: %s\n", message);
        printk("[Hanging now...]\n");
        while (true) {
                asm volatile ("hlt");
        }

}
