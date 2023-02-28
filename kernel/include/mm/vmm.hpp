#pragma once
#include <init/kinfo.hpp>

namespace VMM {
	void InitVMM(KInfo *info);
	void MapMemory(void *virtual_memory, void *physical_memory);
}
