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

	VNode *CreateNode(const inode_t directory, const char name[MAX_NAME_SIZE], property_t flags);
	static VNode *CreateNodeWrapper(void *instance, const inode_t directory, const char name[MAX_NAME_SIZE], property_t flags) {
		return static_cast<RamFS*>(instance)->CreateNode(directory, name, flags);
	}

	VNode *DeleteNode(const inode_t inode);
	static VNode *DeleteNodeWrapper(void *instance, const inode_t inode) {
		return static_cast<RamFS*>(instance)->DeleteNode(inode);
	}

	VNode *GetByInode(const inode_t inode);
	static VNode *GetByInodeWrapper(void *instance, const inode_t inode) {
		return static_cast<RamFS*>(instance)->GetByInode(inode);
	}

	VNode *GetByName(const inode_t directory, const char name[MAX_NAME_SIZE]);
	static VNode *GetByNameWrapper(void *instance, const inode_t directory, const char name[MAX_NAME_SIZE]) {
		return static_cast<RamFS*>(instance)->GetByName(directory, name);
	}
	
	VNode *GetByIndex(const inode_t directory, const size_t index);
	static VNode *GetByIndexWrapper(void *instance, const inode_t directory, const size_t index) {
		return static_cast<RamFS*>(instance)->GetByIndex(directory, index);
	}

	VNode *GetRootNode();
	static VNode *GetRootNodeWrapper(void *instance) {
		return static_cast<RamFS*>(instance)->GetRootNode();
	}


private:
	filesystem_t Descriptor;

	inode_t MaxInodes;
	InodeTableObject *InodeTable;
};
