#pragma once
#include <stdint.h>
#include <init/kinfo.hpp>

extern uint64_t text_start_addr,
		 rodata_start_addr,
		 data_start_addr,
		 text_end_addr,
		 rodata_end_addr,
		 data_end_addr;

void memset(void *start, uint8_t value, uint64_t num);

namespace MEM {
	void Init(KInfo *info);
}
