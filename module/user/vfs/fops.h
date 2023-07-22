#pragma once
#include "typedefs.h"
#include "vnode.h"

struct NodeOperations {
	VNode *(*CreateNode)(void *instance, const inode_t directory, const char name[MAX_NAME_SIZE], property_t flags);
	VNode *(*GetByInode)(void *instance, const inode_t node);
	VNode *(*GetByName)(void *instance, const inode_t directory, const char name[MAX_NAME_SIZE]);
	VNode *(*DeleteNode)(void *instance, const inode_t node);
};

struct FileOperationRequest {
	uint32_t MagicNumber;
	uint16_t Request;

	VNode Data;
}__attribute__((packed));

