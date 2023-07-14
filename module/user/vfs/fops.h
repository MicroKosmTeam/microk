#pragma once
#include "typedefs.h"
#include "vnode.h"

struct NodeOperations {
	inode_t (*CreateNode)(void *instance, const inode_t directory, const char name[256], property_t flags);
	VNode *(*GetByInode)(void *instance, const inode_t node);
	VNode *(*GetByName)(void *instance, const inode_t directory, const char name[256]);
	int (*DeleteNode)(void *instance, const inode_t node);
};

struct FileOperationRequest {
	uint16_t Request;

	struct {
		const char Name[MAX_NAME_SIZE] = { 0 };
		inode_t Inode;
	
		property_t Properties;
		inode_t Directory;
	} Data;
}__attribute__((packed));

