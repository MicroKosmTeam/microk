#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <cdefs.h>
#include <mkmi.h>
#include <mkmi_log.h>
#include <mkmi_memory.h>
#include <mkmi_syscall.h>

#include "vfs/vfs.h"
#include "vfs/ustar.h"
#include "fb/fb.h"

extern "C" uint32_t VendorID = 0xCAFEBABE;
extern "C" uint32_t ProductID = 0xDEADBEEF;

extern "C" size_t OnInit() {
	MKMI_Printf("Requesting initrd.\r\n");

	/* We load the initrd using the kernel file method */
	uintptr_t baseAddress = 0;
	uintptr_t address = 0x8000000000; /* Suitable memory location at address 512GB */
	size_t size;
	Syscall(SYSCALL_FILE_OPEN, "FILE:/initrd.tar", &baseAddress, &size, 0, 0, 0);

	/* Here we check whether it exists 
	 * If it isn't there, just skip this step
	 */
	if (baseAddress != 0 && size != 0) {
		/* Make it accessible in memory */
		Syscall(SYSCALL_MEMORY_MMAP, baseAddress, address, size, 0, 0, 0);

		MKMI_Printf("Loading file initrd.tar from 0x%x with size %dkb.\r\n", address, size / 1024);

		LoadArchive(address);
	} else {
		MKMI_Printf("No initrd found");
	}

	MKMI_Printf("Probing for framebuffer.\r\n");


	uintptr_t fbAddr;
	size_t fbSize;
	Syscall(SYSCALL_FILE_OPEN, "FB:0", &fbAddr, &fbSize, 0, 0, 0);

	/* Here we check whether it exists 
	 * If it isn't there, just skip this step
	 */
	if (fbSize != 0 && fbAddr != 0) {
		/* Make it accessible in memory */
		MKMI_Printf("Mapping...\r\n");
		Syscall(SYSCALL_MEMORY_MMAP, fbAddr, fbAddr, fbSize, 0, 0, 0);
	
		Framebuffer fbData;
		Memcpy(&fbData, fbAddr, sizeof(fbData));

		MKMI_Printf("fb0 starting from 0x%x.\r\n", fbData.Address);
		InitFB(&fbData);

		PrintScreen(" __  __  _                _  __\n"
			    "|  \\/  |(_) __  _ _  ___ | |/ /\n"
			    "| |\\/| || |/ _|| '_|/ _ \\|   < \n"
			    "|_|  |_||_|\\__||_|  \\___/|_|\\_\\\n\n");

		PrintScreen("The user module is starting...\n");
	} else {
		MKMI_Printf("No framebuffer found.\r\n");
	}

	void *bufAddr = 0x8000000000;
	size_t bufSize = 4096 * 6;
	uint32_t bufID;
	Syscall(SYSCALL_MODULE_BUFFER_REGISTER, VendorID, ProductID, bufAddr, bufSize, 0x02, &bufID);
	Memset(bufAddr, 69, bufSize);
	Syscall(SYSCALL_MODULE_BUFFER_UNREGISTER, VendorID, ProductID, bufID, 0x02, 0, 0);

	MKMI_Printf("Done.\r\n");

	//VFSInit();
	
	/* First, initialize VFS data structures */
	/* Instantiate the rootfs (it will be overlayed soon) */
	/* Create a ramfs, overlaying it in the rootfs */
	/* Extract the initrd in the /init directory in the ramfs */
	return 0;
}

extern "C" size_t OnExit() {
	/* Close and destroy all files */
	/* Unmount all filesystems and overlays */
	/* Destroy the rootfs */
	/* Delete VFS data structures */
	return 0;
}
