#include "vfs.h"

#include <mkmi_memory.h>

Inode *RootNode;
Inode *GetRootNode() {
	return RootNode;
}

static void InitRoot() {
	/* Creating the root node. */

	RootNode = Malloc(sizeof(Inode));

	RootNode->FilesystemDescriptor =
	RootNode->InodeNumber =
	RootNode->Properties =
	RootNode->Permissions =
	RootNode->Owner = 
	RootNode->Group = 
	RootNode->Size = 
	RootNode->CreationTime = 
	RootNode->ModificationTime =
	RootNode->AccessTime = 0;
	
	//Strcpy(RootNode->Name, "/\0");

	/* Initializing a ramfs to be mounted to the root node */
}

void VFSInit() {
	
	InitRoot();

}
