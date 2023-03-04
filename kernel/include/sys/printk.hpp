#pragma once
#include <cdefs.h>
#ifdef CONFIG_PRINTK
#include <init/kinfo.hpp>

namespace PRINTK {
	void PrintK(char *format, ...);
#ifdef CONFIG_HW_SERIAL
	void EarlyInit(KInfo *info);
#endif
}
#endif
