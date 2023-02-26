#pragma once
#include <stdint.h>

namespace SCHED {
	void Init();

	uint64_t NewKernelThread(void (*process)(uint64_t tmp));
}
