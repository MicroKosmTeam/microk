#pragma once

#ifdef __cplusplus__
extern "C" {
#endif

typedef enum {
	FS_NODE_FILE,
	FS_NODE_DIRECTORY,
	FS_NODE_SYMLINK,
	FS_NODE_MOUNTPOINT,
} FSNodeType_t;

typedef struct {
	FSNodeType_t NodeType;
} FSNode_t;

#ifdef __cplusplus__
}
#endif
