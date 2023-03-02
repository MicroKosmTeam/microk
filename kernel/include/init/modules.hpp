#pragma once
#include <init/kinfo.hpp>

extern uint64_t *KRNLSYMTABLE;

namespace MODULE {
	void Init(KInfo *info);
}
