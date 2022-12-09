#include <cpu/interrupts.h>
#include <stdint.h>
#include <mm/memory.h>
#include <cpu/cpu.h>

#define IDT_INTERRUPT 0xE
#define IDT_DPL0 0x0
#define IDT_PRESENT 0x80

#define NUM_INTERRUPTS 256

struct idt {
        uint16_t base_l;
        uint16_t cs;
        uint8_t ist;
        uint8_t flags;
        uint16_t base_m;
        uint32_t base_h;
        uint32_t ignored;
} __attribute__((packed)) idt[NUM_INTERRUPTS];

struct {
        uint16_t len;
        struct idt *addr;
} __attribute__((packed)) idtr;

extern uintptr_t isr_table[];
int_handler_t int_handlers[NUM_INTERRUPTS];

registers *page_fault_handler(registers *r);

void idt_set_gate(uint32_t num, uintptr_t vector, uint16_t cs, uint8_t ist, uint8_t flags) {
        idt[num].base_l = vector & 0xFFFF;
        idt[num].base_m = (vector >> 16) & 0xFFFF;
        idt[num].base_h = (vector >> 32) & 0xFFFFFFFF;
        idt[num].cs = cs;
        idt[num].ist = ist;
        idt[num].flags = flags;
}

void interrupt_init() {
        memset(idt, 0, sizeof(idt));
        memset(int_handlers, 0, sizeof(int_handlers));

        for(int i=0; i < NUM_INTERRUPTS; i++) {
                idt_set_gate(i, isr_table[i], 0x8, 0, IDT_PRESENT | IDT_DPL0 | IDT_INTERRUPT);
        }

        idtr.addr = idt;
        idtr.len = sizeof(idt)-1;

        bind_interrupt(14, page_fault_handler);

        load_idt(&idtr);
}

int_handler_t bind_interrupt(uint32_t num, int_handler_t fn) {
        int_handler_t old = int_handlers[num];
        int_handlers[num] = fn;
        return old;
}

registers *page_fault_handler(registers *r) {
        //if(!(r->rflags & (3<<12))) {
                asm volatile ("cli");
#ifdef KCONSOLE_VGA
                vga_print_set_color(PRINT_COLOR_RED, PRINT_COLOR_WHITE);
                vga_print_clear();
#endif
                printk("Unhandled page fault in kernel occurred\n");
                printk("Interrupt number: %d Error code: %d\n", r->int_no, r->err_code);
                printk_registers(r);
                PANIK("Unhandled page fault in kernel occurred");
        //} else {
                printk("Page fault occurred in userspace\n");
        //}
}

registers *int_handler(registers *r) {
        if(int_handlers[r->int_no])
                return int_handlers[r->int_no](r);

        asm volatile ("cli");
#ifdef KCONSOLE_VGA
        vga_print_set_color(PRINT_COLOR_RED, PRINT_COLOR_WHITE);
        vga_print_clear();
#endif

        printk("Unhandled interrupt occurred\n");
        printk("Interrupt number: %d Error code: %d\n", r->int_no, r->err_code);
        printk_registers(r);

        PANIK("Unhandled interrupt occurred");
}
