#pragma once
#include "typedefs.h"
#include "vnode.h"

struct NodeOperations {
	VNode *(*CreateNode)(void *instance, const inode_t directory, const char name[MAX_NAME_SIZE], property_t flags);
	VNode *(*DeleteNode)(void *instance, const inode_t node);

	VNode *(*GetByInode)(void *instance, const inode_t node);
	VNode *(*GetByName)(void *instance, const inode_t directory, const char name[MAX_NAME_SIZE]);
	VNode *(*GetByIndex)(void *instance, const inode_t directory, const size_t index);
	VNode *(*GetRootNode)(void *instance);
};

struct FileOperationRequest {
	uint32_t MagicNumber;
	uint16_t Request;

	union {
		struct {
			inode_t Directory;
			char Name[MAX_NAME_SIZE];
			property_t Flags;
		} CreateNode;

		struct {
			inode_t Node;
		} DeleteNode;

		struct {
			inode_t Node;
		} GetByNode;

		struct {
			inode_t Directory;
			char Name[MAX_NAME_SIZE];
		} GetByName;

		struct {
			inode_t Directory;
			size_t Index;
		} GetByIndex;
	} Data;
}__attribute__((packed));

