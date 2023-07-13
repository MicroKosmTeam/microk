#pragma once
#include "../vfs/typedefs.h"
#include "../vfs/vnode.h"

struct InodeTableObject {
	bool Available = true;

	VNode NodeData;
};

class RamFS {
public:
	RamFS(inode_t maxInodes);
	~RamFS();

	inode_t CreateNode(const char name[256]);
	static inode_t CreateNodeWrapper(void *instance, const char name[256]) {
		return static_cast<RamFS*>(instance)->CreateNode(name);
	}

	VNode *GetNode(const inode_t inode);
	static VNode *GetNodeWrapper(void *instance, const inode_t inode) {
		return static_cast<RamFS*>(instance)->GetNode(inode);
	}
private:
	inode_t MaxInodes;
	InodeTableObject *InodeTable;
};
