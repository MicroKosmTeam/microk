#pragma once
#include <stdint.h>
#include <fs/node.h>

/* FILE
 *  The standard file I/O struct
 *
 */
struct FILE {
	FSNode *node;		// The node that is linked to this file
	uint64_t *buffer;	// The loaded contents of the file
	uint64_t descriptor;	// The descriptor of the file
	uint64_t bufferSize;	// The size read/write buffer
	uint64_t bufferPos;	// Where we last wrote to/read from
} ;

