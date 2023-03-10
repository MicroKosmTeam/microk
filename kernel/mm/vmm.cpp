#include <mm/vmm.hpp>
#include <arch/x64/mm/vmm.hpp>

namespace VMM {
void InitVMM(KInfo *info) {
	x86_64::InitVMM(info);
}

void MapMemory(void *virtual_memory, void *physical_memory) {
	return;
}
}
