#pragma once
#include <init/kutil.h>

namespace x86_64 {
	void LoadGDT();
	void PrepareInterrupts(BootInfo *bootInfo);
	void PreparePaging(BootInfo *bootInfo);
}

