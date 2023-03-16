#pragma once
#include <stddef.h>
#include <stdint.h>
#include <cdefs.h>

#define IsAligned(value, alignment) !(value & (alignment - 1))

struct SourceLocation {
	const char *file;
	uint32_t line;
	uint32_t column;
};

struct TypeDescriptor {
	uint16_t kind;
	uint16_t info;
	char *name;
};

struct TypeMismatchInfo {
	SourceLocation location;
	TypeDescriptor *type;
	uintptr_t alignment;
	uint8_t typeCheckKind;
};

struct OutOfBoundsInfo {
	SourceLocation location;
	TypeDescriptor leftType;
	TypeDescriptor rightType;
};

extern "C" void __ubsan_handle_pointer_overflow();
extern "C" void __ubsan_handle_add_overflow();
extern "C" void __ubsan_handle_sub_overflow();
extern "C" void __ubsan_handle_mul_overflow();
extern "C" void __ubsan_handle_negate_overflow();
extern "C" void __ubsan_handle_divrem_overflow();
extern "C" void __ubsan_handle_load_invalid_value();
extern "C" void __ubsan_handle_shift_out_of_bounds();
extern "C" void __ubsan_handle_builtin_unreachable();
extern "C" void __ubsan_handle_missing_return();
extern "C" void __ubsan_handle_out_of_bounds(void* dataRaw, void* indexRaw);
extern "C" void __ubsan_handle_type_mismatch_v1(TypeMismatchInfo *typeMismatch, uintptr_t pointer);
