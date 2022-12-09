#pragma once
#include <stdint.h>
#include <sys/panik.h>
#include <stdio.h>

typedef struct {
        uint64_t rax;
        uint64_t rbx;
        uint64_t rcx;
        uint64_t rdx;
        uint64_t rsi;
        uint64_t rdi;
        uint64_t rbp;
        uint64_t r8;
        uint64_t r9;
        uint64_t r10;
        uint64_t r11;
        uint64_t r12;
        uint64_t r13;
        uint64_t r14;
        uint64_t r15;

        uint64_t int_no;
        uint64_t err_code;

        uint64_t rip;
        uint64_t cs;
        uint64_t rflags;
        uint64_t rsp;
        uint64_t ss;
} registers;

#define printk_registers(r) \
printk("RAX=0x%x RBX=0x%x RCX=0x%x RDX=0x%x\n", r->rax, r->rbx, r->rcx, r->rdx); \
printk("RSI=0x%x RDI=0x%x RBP=0x%x RSP=0x%x\n", r->rsi, r->rdi, r->rbp, r->rsp); \
printk("R8 =0x%x R9 =0x%x R10=0x%x R11=0x%x\n", r->r8, r->r9, r->r10, r->r11); \
printk("R12=0x%x R13=0x%x R14=0x%x R15=0x%x\n", r->r12, r->r13, r->r14, r->r15); \
printk("RIP=0x%x RFL=0x%x\n", r->rip, r->rflags); \
printk("CS=0x%x SS=0x%x\n", r->cs, r->ss); \
printk("CR0=0x%x CR2=0x%x CR3=0x%x CR4=0x%x\n", read_cr0(), read_cr2(), read_cr3(), read_cr4());


typedef registers *(*int_handler_t)(registers *);

void interrupt_init();
int_handler_t bind_interrupt(uint32_t num, int_handler_t fn);
void isr_return(registers *);
