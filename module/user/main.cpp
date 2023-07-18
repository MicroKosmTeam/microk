#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <cdefs.h>
#include <mkmi.h>
#include <mkmi_log.h>
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

struct Message {
	uint32_t SenderVendorID : 32;
	uint32_t SenderProductID : 32;

	size_t MessageSize : 64;
}__attribute__((packed));

VirtualFilesystem *vfs;
RamFS *rootRamfs;
filesystem_t ramfsDesc;

extern "C" size_t OnMessage() {
	uintptr_t bufAddr = 0xF000000000;

	Message *msg = bufAddr;
	const uint32_t *signature = bufAddr + 128;

	MKMI_Printf("Message:\r\n"
		    " - Sender: %x by %x\r\n"
		    " - Size: %d\r\n"
		    " - Magic Number: %x\r\n",
		    msg->SenderProductID, msg->SenderVendorID,
		    msg->MessageSize, *signature);

	if(*signature != FILE_REQUEST_MAGIC_NUMBER) return 0;

	FileOperationRequest *request = bufAddr + 128;

	void *response = vfs->DoFilesystemOperation(1, request);
	
	Memset(bufAddr, 0, msg->MessageSize);

	if (response == NULL) return 0;
	
	request->MagicNumber = FILE_RESPONSE_MAGIC_NUMBER;
	request->Request = 0;
	Memcpy(&request->Data, response, sizeof(VNode));

	rootRamfs->ListDirectory(0);
	rootRamfs->ListDirectory(1);

	Syscall(SYSCALL_MODULE_MESSAGE_SEND, 0xCAFEBABE, 0xB830C0DE, 1, 0, 1, 1024);

	return 0;
}

extern "C" size_t OnSignal() {
	MKMI_Printf("Signal!\r\n");
	return 0;
}

void InitrdInit();
void FBInit();

extern "C" size_t OnInit() {
	InitrdInit();
	FBInit();

	uintptr_t bufAddr = 0xF000000000;
	size_t bufSize = 4096 * 2;
	uint32_t bufID;
	bufID = Syscall(SYSCALL_MODULE_BUFFER_REGISTER, bufAddr, bufSize, 0x02, 0, 0, 0);

//	Syscall(SYSCALL_MODULE_BUFFER_UNREGISTER, bufID, 0, 0, 0, 0 ,0);

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
	//while(true);

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
