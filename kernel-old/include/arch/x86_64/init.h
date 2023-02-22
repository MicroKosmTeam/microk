#pragma once
#include <init/kutil.h>

namespace x86_64 {
	void LoadGDT();
	void SetupPaging(BootInfo *bootInfo);
	void PrepareInterrupts(BootInfo *bootInfo);
}

