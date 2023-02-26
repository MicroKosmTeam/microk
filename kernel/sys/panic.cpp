#include <sys/printk.hpp>

__attribute__((noreturn))
void Panic(const char *message, const char *file, const char *function, unsigned int line) {
        asm volatile ("cli"); // We don't want interrupts while we are panicking

	using namespace PRINTK;

        // Printing the panic message
	PrintK("\n\n!! PANIK!! \n");
        PrintK("Irrecoverable error in the kernel.\n\n");
        PrintK("%s in function %s at line %d\n", file, function, line);
        PrintK("Cause: %s\n", message);
        PrintK("[Hanging now...]\n");

        while (true) {
                // Halting forever
                asm volatile ("hlt");
        }

}
