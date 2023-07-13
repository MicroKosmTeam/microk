#include "ramfs.h"

#include <mkmi_memory.h>

RamFS::RamFS(inode_t maxInodes) {
	MaxInodes = maxInodes;
	InodeTable = new InodeTableObject[MaxInodes];
}

RamFS::~RamFS() {
	delete InodeTable;
}

inode_t RamFS::CreateNode(const char name[256]) {
	for (size_t i = 1; i < MaxInodes; ++i) {
		if(InodeTable[i].Available) {
			Memcpy(InodeTable[i].NodeData.Name, name, 256);
			InodeTable[i].Available = false;

			return i;
		}
	}
	
	return 0;
}
VNode *RamFS::GetNode(const inode_t inode) {
	if (inode > MaxInodes || inode == 0) return NULL;

	InodeTableObject *node = &InodeTable[inode];

	if(node->Available) return NULL;

	VNode *vnode = &node->NodeData;

	return vnode;

}
