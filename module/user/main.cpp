#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <cdefs.h>
#include <mkmi.h>
#include <mkmi_log.h>
#include <mkmi_exit.h>
#include <mkmi_memory.h>
#include <mkmi_syscall.h>

#include "vfs/fops.h"
#include "vfs/typedefs.h"
#include "vfs/vfs.h"
#include "vfs/ustar.h"
#include "ramfs/ramfs.h"
#include "fb/fb.h"

extern "C" uint32_t VendorID = 0xCAFEBABE;
extern "C" uint32_t ProductID = 0xDEADBEEF;

void InitrdInit();
void FBInit();

/* The header will always be 128 bytes */
struct Message {
	uint32_t SenderVendorID : 32;
	uint32_t SenderProductID : 32;

	size_t MessageSize : 64;
}__attribute__((packed));

void MessageHandler() {
	Message *msg = 0x700000000000;
	uint32_t *data = (uint32_t*)((uintptr_t)msg + 128);

	MKMI_Printf("Message at        0x%x\r\n"
		    " Sender Vendor ID:  %x\r\n"
		    " Sender Product ID: %x\r\n"
		    " Message Size:      %d\r\n"
		    " Data:              %d\r\n",
		    msg,
		    msg->SenderVendorID,
		    msg->SenderProductID,
		    msg->MessageSize,
		    *data);

	uintptr_t bufAddr = 0xE000000000;
	size_t bufSize = 4096 * 2;
	uint32_t bufID = *data;

	Syscall(SYSCALL_MODULE_BUFFER_MAP, bufAddr, bufID, 0, 0, 0, 0);
	MKMI_Printf("Buffer data: %d\r\n", *(uint32_t*)bufAddr);

	VMFree(data, msg->MessageSize);

	_return(0);
}

extern "C" size_t OnInit() {
	Syscall(SYSCALL_MODULE_MESSAGE_HANDLER, MessageHandler, 0, 0, 0, 0, 0);

	InitrdInit();
	FBInit();

	Syscall(SYSCALL_MODULE_SECTION_REGISTER, "VFS", VendorID, ProductID, 0, 0 ,0);

	VirtualFilesystem *vfs;
	RamFS *rootRamfs;
	filesystem_t ramfsDesc;

	vfs = new VirtualFilesystem();
	rootRamfs = new RamFS(2048);

	NodeOperations *ramfsOps = new NodeOperations;
	
	ramfsOps->CreateNode = rootRamfs->CreateNodeWrapper;
	ramfsOps->GetByInode = rootRamfs->GetByInodeWrapper;
	ramfsOps->GetByName = rootRamfs->GetByNameWrapper;
	ramfsOps->DeleteNode = rootRamfs->DeleteNodeWrapper;

	ramfsDesc = vfs->RegisterFilesystem(0, 0, rootRamfs, ramfsOps);

	rootRamfs->SetDescriptor(ramfsDesc);

	FileOperationRequest request;
	request.Request = NODE_CREATE;
	request.Data.Directory = 0;
	request.Data.Properties = NODE_PROPERTY_DIRECTORY;
	Memcpy(request.Data.Name, "dev", 3);

	VNode *devNode = vfs->DoFilesystemOperation(ramfsDesc, &request);

	request.Request = NODE_CREATE;
	request.Data.Directory = devNode->Inode;
	request.Data.Properties = NODE_PROPERTY_DIRECTORY;
	Memcpy(request.Data.Name, "tty", 3);

	VNode *ttyNode = vfs->DoFilesystemOperation(ramfsDesc, &request);

	request.Request = NODE_CREATE;
	request.Data.Directory = ttyNode->Inode;
	request.Data.Properties = NODE_PROPERTY_FILE;
	Memcpy(request.Data.Name, "tty1", 4);

	VNode *ttyFile = vfs->DoFilesystemOperation(ramfsDesc, &request);

	request.Request = NODE_CREATE;
	request.Data.Directory = ttyNode->Inode;
	request.Data.Properties = NODE_PROPERTY_FILE;
	Memcpy(request.Data.Name, "tty2", 4);

	vfs->DoFilesystemOperation(ramfsDesc, &request);

	request.Request = NODE_CREATE;
	request.Data.Directory = devNode->Inode;
	request.Data.Properties = NODE_PROPERTY_FILE;
	Memcpy(request.Data.Name, "kmsg", 4);

	vfs->DoFilesystemOperation(ramfsDesc, &request);

	request.Request = NODE_GET;
	request.Data.Inode = ttyFile->Inode;

	VNode *node = vfs->DoFilesystemOperation(ramfsDesc, &request);
	MKMI_Printf("Got object: %s.\r\n", node->Name);

	rootRamfs->ListDirectory(0);
	rootRamfs->ListDirectory(devNode->Inode);
	rootRamfs->ListDirectory(ttyNode->Inode);


	uintptr_t bufAddr = 0xF000000000;
	size_t bufSize = 4096 * 2;
	uint32_t bufID;
	Syscall(SYSCALL_MODULE_BUFFER_CREATE, bufSize, 0x02, &bufID, 0, 0, 0);
	Syscall(SYSCALL_MODULE_BUFFER_MAP, bufAddr, bufID, 0, 0, 0, 0);
	*(uint32_t*)bufAddr = 69;
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

void InitrdInit() {
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
		VMMap(baseAddress, address, size, 0);

		MKMI_Printf("Loading file initrd.tar from 0x%x with size %dkb.\r\n", address, size / 1024);

		LoadArchive(address);
	} else {
		MKMI_Printf("No initrd found");
	}
}

void FBInit() {
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
		VMMap(fbAddr, fbAddr, fbSize, 0);
	
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
}
