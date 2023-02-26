#pragma once
#include <arch/x64/mm/vmm.hpp>
#include <init/kinfo.hpp>

namespace VMM {
	void InitVMM(KInfo *info) {
		x86_64::InitVMM(info);
	}
}
