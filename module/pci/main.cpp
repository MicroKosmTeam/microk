#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <cdefs.h>
#include <mkmi.h>
#include <mkmi_log.h>
#include <mkmi_memory.h>
#include <mkmi_syscall.h>

#include "pci/pci.h"

extern "C" uint32_t VendorID = 0xCAFEBABE;
extern "C" uint32_t ProductID = 0xB830C0DE;

extern "C" size_t OnInit() {
	uint32_t pid = 0, vid = 0;
	Syscall(SYSCALL_MODULE_SECTION_GET, "VFS", &vid, &pid, 0, 0 ,0);
	MKMI_Printf("VFS -> VID: %x PID: %x\r\n", vid, pid);

	uintptr_t bufAddr = 0xD000000000;
	size_t bufSize = 4096 * 2;
	uint32_t bufID;
	Syscall(SYSCALL_MODULE_BUFFER_CREATE, bufSize, 0x02, &bufID, 0, 0, 0);
	Syscall(SYSCALL_MODULE_BUFFER_MAP, bufAddr, bufID, 0, 0, 0, 0);
	*(uint32_t*)bufAddr = 69;

	uint8_t msg[256];
	uint32_t *data = (uint32_t*)((uintptr_t)msg + 128);
	*data = bufID;

	Syscall(SYSCALL_MODULE_MESSAGE_SEND, vid, pid, msg, 256, 0 ,0);

	Free(msg);

	void *mcfg;
	size_t mcfgSize;
	Syscall(SYSCALL_FILE_OPEN, "ACPI:MCFG", &mcfg, &mcfgSize, 0, 0, 0);

	/* Here we check whether it exists 
	 * If it isn't there, just skip this step
	 */
	if (mcfg != 0 && mcfgSize != 0) {
		/* Make it accessible in memory */
		Syscall(SYSCALL_MEMORY_MMAP, mcfg, mcfg, mcfgSize, 0, 0, 0);

		MKMI_Printf("MCFG at 0x%x, size: %d\r\n", mcfg, mcfgSize);

		Syscall(SYSCALL_MODULE_SECTION_REGISTER, "PCI", VendorID, ProductID, 0, 0 ,0);
		Syscall(SYSCALL_MODULE_SECTION_REGISTER, "PCI", VendorID, ProductID, 0, 0 ,0);

		EnumeratePCI(mcfg);
	} else {
		MKMI_Printf("No MCFG found.\r\n");
		return 1;
	}

	return 0;
}

extern "C" size_t OnExit() {

	return 0;
}
