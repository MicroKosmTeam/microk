#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <mkmi.h>

extern "C" volatile MKMI_Module ModuleInfo {
	.ID = {
		.Name = "MicroKosm Hello Module",
		.Author = "Mutta Filippo <filippo.mutta@gmail.com>",
		.Codename = 0xDEADBEEF,
		.Writer = 0xCAFEBABE
	},
	.Version = {
		.Major = 0,
		.Minor = 0,
		.Feature = 1,
		.Patch = 0
	}
};

extern "C" uint64_t ModuleInit() {
	return 0;
}

extern "C" uint64_t ModuleDeinit() {
	return 0;
}
