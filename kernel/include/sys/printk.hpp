#pragma once
#include <init/kinfo.hpp>

namespace PRINTK {
	void PrintK(char *format, ...);
	void EarlyInit(KInfo *info);
}
