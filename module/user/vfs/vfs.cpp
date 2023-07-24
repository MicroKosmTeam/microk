#include "vfs.h"
#include "fops.h"
#include "typedefs.h"

#include <mkmi_memory.h>
#include <mkmi_string.h>
#include <mkmi_log.h>

VirtualFilesystem::VirtualFilesystem() {
	BaseNode = new RegisteredFilesystemNode;
	BaseNode->FS = NULL;
	BaseNode->Next = NULL;
}


VirtualFilesystem::~VirtualFilesystem() {
	delete BaseNode;
}
	
filesystem_t VirtualFilesystem::RegisterFilesystem(uint32_t vendorID, uint32_t productID, void *instance, NodeOperations *ops) {
	/* It's us */
	Filesystem *fs = new Filesystem;

	fs->FSDescriptor = GetFSDescriptor();
	fs->OwnerVendorID = vendorID;
	fs->OwnerProductID = productID;
	fs->Instance = instance;
	fs->Operations = ops;

	RegisteredFilesystemNode *node = AddNode(fs);

	MKMI_Printf("Registered filesystem (ID: %x, VID: %x, PID: %x)\r\n",
			node->FS->FSDescriptor,
			node->FS->OwnerVendorID,
			node->FS->OwnerProductID);

	return fs->FSDescriptor;
}
	
#define IF_IS_OURS( x ) \
	if (node->FS->OwnerVendorID == 0 && node->FS->OwnerProductID == 0) return x

uintmax_t VirtualFilesystem::DoFilesystemOperation(filesystem_t fs, FileOperationRequest *request) {
	bool found = false;
	RegisteredFilesystemNode *previous; 
	RegisteredFilesystemNode *node = FindNode(fs, &previous, &found);

	if (node == NULL || !found) return 0;
	if (request == NULL) return 0;

	switch(request->Request) {
		case NODE_CREATE:
			IF_IS_OURS(node->FS->Operations->CreateNode(node->FS->Instance, request->Data.CreateNode.Directory, request->Data.CreateNode.Name, request->Data.CreateNode.Flags));
			break;
		case NODE_DELETE:
			IF_IS_OURS(node->FS->Operations->DeleteNode(node->FS->Instance, request->Data.DeleteNode.Node));
			break;
		case NODE_GETBYNODE:
			IF_IS_OURS(node->FS->Operations->GetByInode(node->FS->Instance, request->Data.GetByNode.Node));
			break;
		case NODE_GETBYNAME:
			IF_IS_OURS(node->FS->Operations->GetByName(node->FS->Instance, request->Data.GetByName.Directory, request->Data.GetByName.Name));
			break;
		case NODE_GETBYINDEX:
			IF_IS_OURS(node->FS->Operations->GetByIndex(node->FS->Instance, request->Data.GetByIndex.Directory, request->Data.GetByIndex.Index));
			break;
		case NODE_GETROOT:
			IF_IS_OURS(node->FS->Operations->GetRootNode(node->FS->Instance));
			break;
		default:
			return 0;
	}

	return 0;
}

void VirtualFilesystem::SetRootFS(filesystem_t fs) {
	FileOperationRequest request;
	request.Request = NODE_GETROOT;
	VNode *rootNode = DoFilesystemOperation(fs, &request);
	
	if(rootNode != NULL || rootNode != -1)
		RootNode = rootNode;
}

VNode *VirtualFilesystem::ProgressPath(VNode *current, const char *nextName) {
	FileOperationRequest request;

	request.Request = NODE_GETBYNAME;
	request.Data.GetByName.Directory = current->Inode;
	Strcpy(request.Data.GetByName.Name, nextName);

	VNode *result = DoFilesystemOperation(current->FSDescriptor, &request);

	return result;
}

VNode *VirtualFilesystem::ResolvePath(const char *path) {
	VNode *current = RootNode;

	const char *id = Strtok(path, "/");
	if(id == NULL) return current;

	while(true) {
		current = ProgressPath(current, id);
		if(current == NULL || current == -1) return NULL;

		id = Strtok(NULL, "/");
		if(id == NULL) return current;
	}

	return NULL;
}

RegisteredFilesystemNode *VirtualFilesystem::AddNode(Filesystem *fs) {
	bool found = false;
	RegisteredFilesystemNode *node, *prev;
	node = FindNode(fs->FSDescriptor, &prev, &found);

	/* Already present, return this one */
	if(found) return node;

	/* If not, prev is now our last node. */
	RegisteredFilesystemNode *newNode = new RegisteredFilesystemNode;
	newNode->FS = fs;
	newNode->Next = NULL;

	prev->Next = newNode;
	
	return prev->Next;
}

void VirtualFilesystem::RemoveNode(filesystem_t fs) {
	bool found = false;
	RegisteredFilesystemNode *previous; 
	RegisteredFilesystemNode *node = FindNode(fs, &previous, &found);

	/* This issue shall be reported */
	if(!found) return;

	previous->Next = node->Next;

	delete node->FS;
	delete node;
}

RegisteredFilesystemNode *VirtualFilesystem::FindNode(filesystem_t fs, RegisteredFilesystemNode **previous, bool *found) {
	RegisteredFilesystemNode *node = BaseNode->Next, *prev = BaseNode;

	if (node == NULL) {
		*previous = prev;
		*found = false;
		return NULL;
	}


	while(true) {
		if (node->FS->FSDescriptor == fs) {
			*found = true;
			*previous = prev;
			return node;
		}

		prev = node;
		if (node->Next == NULL) break;
		node = node->Next;
	}

	*previous = prev;
	*found = false;
	return NULL;
}

