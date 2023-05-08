#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <mkmi.h>
#include <mkmi_log.h>
#include <mkmi_memory.h>

#include "vfs/vfs.h"

extern "C" const volatile MKMI_Module ModuleInfo =  {
	.ID = {
		.Name = "MicroKosm VFS Module",
		.Author = "Mutta Filippo <filippo.mutta@gmail.com>",
		.Codename = 0xDEADBEEF,
		.Writer = 0xCAFEBABE
	},
	.Version = {
		.Major = 0,
		.Minor = 0,
		.Feature = 1,
		.Patch = 0
	},
};

extern "C" uint64_t ModuleInit() {
	MKMI_Printf("Module initializing...\r\n");

	return 0;
}

extern "C" uint64_t ModuleDeinit() {
	MKMI_Printf("Goodbye!\r\n");

	return 0;
}
