#pragma once
#include <stdint.h>
#include <stddef.h>

#define MAX_NAME_SIZE 512

#define FOP_CREATE     0xF000
#define FOP_DELETE     0xF001
#define FOP_MKDIR      0xF002
#define FOP_RMDIR      0xF003
#define FOP_FINDINDIR  0xF004

struct FileOperationRequest {
	uint16_t Request;

	const char Name[MAX_NAME_SIZE];

	union {
		struct {
		}__attribute__((packed)) MkdirOp;

		struct {
		}__attribute__((packed)) RmdirOp;

		struct {
		}__attribute__((packed)) FindInDirOp;

	};
}__attribute__((packed));

struct FileOperationResult {

}__attribute__((packed));
