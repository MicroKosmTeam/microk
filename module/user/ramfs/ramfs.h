#pragma once
#include "../vfs/typedefs.h"
#include "../vfs/vnode.h"

struct InodeTableObject;

struct DirectoryVNodeTable {
	InodeTableObject *Elements[NODES_IN_VNODE_TABLE];

	DirectoryVNodeTable *NextTable;
};

struct InodeTableObject {
	bool Available = true;

	VNode NodeData;
	DirectoryVNodeTable *DirectoryTable;
};

class RamFS {
public:
	RamFS(inode_t maxInodes);
	~RamFS();

	void SetDescriptor(filesystem_t desc) {
		if (Descriptor != 0) return;
		Descriptor = desc;

		if (InodeTable == NULL) return;
		InodeTable[0].NodeData.FSDescriptor = Descriptor;
	}

	int ListDirectory(const inode_t directory);

	inode_t CreateNode(const inode_t directory, const char name[256], property_t flags);
	static inode_t CreateNodeWrapper(void *instance, const inode_t directory, const char name[256], property_t flags) {
		return static_cast<RamFS*>(instance)->CreateNode(directory, name, flags);
	}

	VNode *GetByInode(const inode_t inode);
	static VNode *GetByInodeWrapper(void *instance, const inode_t inode) {
		return static_cast<RamFS*>(instance)->GetByInode(inode);
	}

	inode_t GetByName(const inode_t directory, const char name[256]);
	static inode_t GetByNameWrapper(void *instance, const inode_t inode, const char name[256]) {
		return static_cast<RamFS*>(instance)->GetByName(inode, name);
	}

	int DeleteNode(const inode_t inode);
	static inode_t DeleteNodeWrapper(void *instance, const inode_t inode) {
		return static_cast<RamFS*>(instance)->DeleteNode(inode);
	}
private:
	filesystem_t Descriptor;

	inode_t MaxInodes;
	InodeTableObject *InodeTable;
};
