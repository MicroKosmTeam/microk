#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <mkmi.h>

extern "C" const volatile MKMI_Module ModuleInfo =  {
	.ID = {
		.Name = "MicroKosm Hello Module",
		.Author = "Mutta Filippo <filippo.mutta@gmail.com>",
		.Codename = 0x6E7F11E,
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
	ModuleInfo.PrintK("Module initializing...\r\n");
	ModuleInfo.PrintK("Initializing the VFS!\r\n");

	return 0;
}

extern "C" uint64_t ModuleDeinit() {
	ModuleInfo.PrintK("Goodbye!\r\n");

	return 0;
}
