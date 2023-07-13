#pragma once
#include "typedefs.h"
#include "vnode.h"

struct NodeOperations {
	inode_t (*CreateNode)(void *instance, const char [256]);
	VNode *(*GetNode)(void *instance, const inode_t node);
	int (*DeleteNode)(void *instance, const inode_t node);
};

struct FileOperationRequest {
	uint16_t Request;

	union {
		const char Name[MAX_NAME_SIZE] = { 0 };
		inode_t Inode;
	} Data;
}__attribute__((packed));
