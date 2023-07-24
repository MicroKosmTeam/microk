#pragma once
#include <stdint.h>
#include <stddef.h>

#include "typedefs.h"
#include "vnode.h"
#include "fs.h"
#include "fops.h"

struct RegisteredFilesystemNode {
	Filesystem *FS;

	RegisteredFilesystemNode *Next;
};

class VirtualFilesystem {
public:
	VirtualFilesystem();
	~VirtualFilesystem();

	filesystem_t RegisterFilesystem(uint32_t vendorID, uint32_t productID, void *instance, NodeOperations *ops);
	uintmax_t DoFilesystemOperation(filesystem_t fs, FileOperationRequest *request);
	void UnregisterFilesystem(filesystem_t fs);

	void SetRootFS(filesystem_t fs);
	VNode *ResolvePath(const char *path);
private:
	RegisteredFilesystemNode *AddNode(Filesystem *fs);
	void RemoveNode(filesystem_t fs);
	RegisteredFilesystemNode *FindNode(filesystem_t fs, RegisteredFilesystemNode **previous, bool *found);

	VNode *RootNode;
	
	VNode *ProgressPath(VNode *current, const char *nextName);

	RegisteredFilesystemNode *BaseNode;

	filesystem_t MaxFSDescriptor = 0;
	filesystem_t GetFSDescriptor() { return ++MaxFSDescriptor; }
};
