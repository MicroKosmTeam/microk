#pragma once
#include <stddef.h>
#include <stdint.h>
#include <cdefs.h>

#define is_aligned(value, alignment) !(value & (alignment - 1))

struct source_location {
    const char *file;
    uint32_t line;
    uint32_t column;
};

struct type_descriptor {
    uint16_t kind;
    uint16_t info;
    char *name;
};

struct type_mismatch_info {
    struct source_location location;
    struct type_descriptor *type;
    uintptr_t alignment;
    uint8_t type_check_kind;
};

struct out_of_bounds_info {
    struct source_location location;
    struct type_descriptor left_type;
    struct type_descriptor right_type;
};

#ifdef __cplusplus
extern "C" {
#endif
void __stack_chk_fail(void);
void UnwindStack(int MaxFrame);
void __ubsan_handle_pointer_overflow();
void __ubsan_handle_add_overflow();
void __ubsan_handle_sub_overflow();
void __ubsan_handle_mul_overflow();
void __ubsan_handle_negate_overflow();
void __ubsan_handle_divrem_overflow();
void __ubsan_handle_load_invalid_value();
void __ubsan_handle_shift_out_of_bounds();
void __ubsan_handle_builtin_unreachable();
void __ubsan_handle_missing_return();
void __ubsan_handle_out_of_bounds(void* data_raw, void* index_raw);
void __ubsan_handle_type_mismatch_v1(struct type_mismatch_info *type_mismatch, uintptr_t pointer);
#ifdef __cplusplus
}
#endif

#ifndef PREFIX
        #define RENAME(f)
#else
        #define RENAME(f) PREFIX ## f
#endif

