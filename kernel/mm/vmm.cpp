#include <mm/vmm.hpp>
#include <arch/x64/mm/pagetable.hpp>
#include <arch/x64/mm/vmm.hpp>

namespace VMM {
void InitVMM(KInfo *info) {
	x86_64::InitVMM(info);
}

void MapMemory(void *virtual_memory, void *physical_memory) {
	PT_Flag flags[128];
	bool flagStatus[128];
	uint64_t flagNumber;
	flagNumber = 1;
	flags[0] = PT_Flag::ReadWrite;
	flagStatus[0] = true;
	GlobalPageTableManager->MapMemory((void*)virtual_memory, (void*)physical_memory, flagNumber, flags, flagStatus);
}
}
