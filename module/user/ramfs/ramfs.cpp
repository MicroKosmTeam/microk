#include "ramfs.h"

#include <mkmi_memory.h>
#include <mkmi_string.h>
#include <mkmi_log.h>

RamFS::RamFS(inode_t maxInodes) {
	Descriptor = 0;
	MaxInodes = maxInodes;
	InodeTable = new InodeTableObject[MaxInodes];

	InodeTableObject *node = &InodeTable[0];
	node->Available = false;

	node->NodeData.FSDescriptor = 0;
	node->NodeData.Properties = NODE_PROPERTY_DIRECTORY;
	node->NodeData.Inode = 0;

	node->DirectoryTable = new DirectoryVNodeTable;
	node->DirectoryTable->NextTable = NULL;

	Memset(node->DirectoryTable->Elements, 0, NODES_IN_VNODE_TABLE * sizeof(uintptr_t));
}

RamFS::~RamFS() {
	delete InodeTable;
}

int RamFS::ListDirectory(const inode_t directory) {
	if (directory > MaxInodes) return 0;

	InodeTableObject *dir = &InodeTable[directory];

	if(dir->Available) return 0;
	if(!(dir->NodeData.Properties & NODE_PROPERTY_DIRECTORY)) return 0;
/*
	if(dir->NodeData.Properties & NODE_PROPERTY_MOUNTPOINT) {
		return -1;
	} 

	if(dir->NodeData.Properties & NODE_PROPERTY_SYMLINK) {
		return -1;
	}
*/
	if(dir->DirectoryTable == NULL) return 0;

	DirectoryVNodeTable *table = dir->DirectoryTable;

	MKMI_Printf("   Name   Inode\r\n");
	while(true) {
		for (size_t i = 0; i < NODES_IN_VNODE_TABLE; ++i) {
			if(table->Elements[i] == NULL || table->Elements[i] == -1) continue;

			MKMI_Printf(" -> %s   %d\r\n", table->Elements[i]->NodeData.Name, table->Elements[i]->NodeData.Inode);
		}

		if(table->NextTable == NULL) return 0;
		table = table->NextTable;
	}

	return 0;
}

VNode *RamFS::CreateNode(const inode_t directory, const char name[MAX_NAME_SIZE], property_t flags) {
	if (directory > MaxInodes) return 0;
	if (flags == 0) return 0;
				
	InodeTableObject *dir = &InodeTable[directory];

	if(dir->Available) return 0;
	if(!(dir->NodeData.Properties & NODE_PROPERTY_DIRECTORY)) return 0;
/*
	if(dir->NodeData.Properties & NODE_PROPERTY_MOUNTPOINT) {
		return 0;
	} 

	if(dir->NodeData.Properties & NODE_PROPERTY_SYMLINK) {
		return 0;
	}
*/
	if(dir->DirectoryTable == NULL) return 0;
			
	DirectoryVNodeTable *table = dir->DirectoryTable;

	for (size_t i = 1; i < MaxInodes; ++i) {
		if(InodeTable[i].Available) {
			InodeTable[i].Available = false;
			
			InodeTable[i].NodeData.FSDescriptor = Descriptor;

			Memcpy(InodeTable[i].NodeData.Name, name, MAX_NAME_SIZE);
			InodeTable[i].NodeData.Inode = i;

			if(flags & NODE_PROPERTY_DIRECTORY) {
				InodeTable[i].NodeData.Properties |= NODE_PROPERTY_DIRECTORY; 
				InodeTable[i].DirectoryTable = new DirectoryVNodeTable;
				InodeTable[i].DirectoryTable->NextTable = NULL;
				Memset(InodeTable[i].DirectoryTable->Elements, 0, NODES_IN_VNODE_TABLE * sizeof(uintptr_t));
			} else if (flags & NODE_PROPERTY_FILE) {
				InodeTable[i].NodeData.Properties |= NODE_PROPERTY_FILE;
				InodeTable[i].DirectoryTable = NULL;
			}

			bool found = false;
			while(true) {
				for (size_t j = 0; j < NODES_IN_VNODE_TABLE; ++j) {
					if(table->Elements[j] != NULL && table->Elements[j] != -1) continue;

					table->Elements[j] = &InodeTable[i];
					found = true;
					break;
				}

				if(found) break;

				if(table->NextTable == NULL) return 0;
				table = table->NextTable;
			}

			return &InodeTable[i].NodeData;
		}
	}
	
	return 0;
}

VNode *RamFS::DeleteNode(const inode_t inode) {
	if (inode > MaxInodes) return 0;

	InodeTableObject *node = &InodeTable[inode];

	if(node->Available) return 0;

	return 0;
}

VNode *RamFS::GetByInode(const inode_t inode) {
	if (inode > MaxInodes) return 0;

	InodeTableObject *node = &InodeTable[inode];

	if(node->Available) {
		return 0;
	}

	VNode *vnode = &node->NodeData;

	return vnode;

}

VNode *RamFS::GetByName(const inode_t directory, const char name[MAX_NAME_SIZE]) {
	if (directory > MaxInodes) return 0;

	InodeTableObject *dir = &InodeTable[directory];
	
	if(dir->Available) return 0;
	if((dir->NodeData.Properties & NODE_PROPERTY_DIRECTORY) == 0) return 0;

	if(dir->DirectoryTable == NULL) return 0;

	DirectoryVNodeTable *table = dir->DirectoryTable;

	while(true) {
		for (size_t i = 0; i < NODES_IN_VNODE_TABLE; ++i) {
			if(table->Elements[i] == NULL || table->Elements[i] == -1) continue;

			if(Strcmp(name, table->Elements[i]->NodeData.Name) == 0) {
				return &table->Elements[i]->NodeData;
			}
		}

		if(table->NextTable == NULL) {
			return 0;
		}
		table = table->NextTable;
	}

	return 0;
}
	
VNode *RamFS::GetByIndex(const inode_t directory, const size_t index) {
	if (directory > MaxInodes) return 0;

	InodeTableObject *dir = &InodeTable[directory];

	if(dir->Available) return 0;
	if(!(dir->NodeData.Properties & NODE_PROPERTY_DIRECTORY)) return 0;

	/* Handle mountpoints first */
	if(dir->NodeData.Properties & NODE_PROPERTY_MOUNTPOINT) {
		return 0;
	} 

	/* Handle symlinks then */
	if(dir->NodeData.Properties & NODE_PROPERTY_SYMLINK) {
		return 0;
	}

	if(dir->DirectoryTable == NULL) return 0;

	DirectoryVNodeTable *table = dir->DirectoryTable;

	size_t tableIndex = index % NODES_IN_VNODE_TABLE;
	size_t tablesToCross = (index - tableIndex) / NODES_IN_VNODE_TABLE;

	for (size_t i = 0; i < tablesToCross; ++i) {
		if(table->NextTable == NULL) return 0;
		table = table->NextTable;
	}

	if(table->Elements[tableIndex] == NULL || table->Elements[tableIndex] == -1) return 0;
	if(table->Elements[tableIndex]->Available) return 0;

	return &table->Elements[tableIndex]->NodeData;
}
	
VNode *RamFS::GetRootNode() {
	VNode *node = &InodeTable[0].NodeData;
	return node;
}
