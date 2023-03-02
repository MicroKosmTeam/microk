#pragma once
#include <stdint.h>

struct Registers {
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

	uint64_t intNo;
	uint64_t errCode;

	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

typedef Registers *(*IntHandler)(Registers *);

extern "C" void isrReturn(Registers *);

namespace x86_64 {
	void InterruptInit();
	IntHandler BindInterrupt(uint32_t num, IntHandler fn);
}
