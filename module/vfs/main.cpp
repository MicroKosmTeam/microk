#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <cdefs.h>
#include <mkmi.h>
#include <mkmi_log.h>
#include <mkmi_memory.h>
#include <mkmi_syscall.h>

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
	.OnInterrupt = NULL,
	.OnMessage = NULL
};

#define PREFIX "[VFS] "

extern "C" size_t OnInit() {
	MKMI_Printf(PREFIX "MicroKosm VFS Module is initializing...\r\n");

	MKMI_Printf(PREFIX "Hello world!\r\n");

	return 0;
}

extern "C" size_t OnExit() {
	MKMI_Printf(PREFIX "MicroKosm VFS Module is quitting...\r\n");

	return 0;
}
