#include <stdint.h>
#include <stddef.h>
#include <sys/panik.h>
#include <fs/vfs.h>

const char *Type_Check_Kinds[] = {
    "load of",
    "store to",
    "reference binding to",
    "member access within",
    "member call on",
    "constructor call on",
    "downcast of",
    "downcast of",
    "upcast of",
    "cast to virtual base of",
};

static void print_location(struct source_location *location) {
    dprintf(" file: %s\n line: %d\n column: %d\n",
         location->file, location->line, location->column);
}

void __ubsan_handle_missing_return() {
}

void __ubsan_handle_divrem_overflow() {
}

void __ubsan_handle_load_invalid_value() {
}

void __ubsan_handle_shift_out_of_bounds() {
}

void __ubsan_handle_builtin_unreachable() {
}

void __ubsan_handle_mul_overflow() {
}

void __ubsan_handle_negate_overflow() {
}

void __ubsan_handle_out_of_bounds(void* data_raw, void* index_raw) {
        out_of_bounds_info *data = (out_of_bounds_info*)data_raw;
        uintptr_t index = (uintptr_t)index_raw;
        (void)index;

        dprintf("\n!!! UBSAN !!!\n"
                "Out of bounds!\n");

        print_location(&data->location);
}

void __ubsan_handle_pointer_overflow() {
}
void __ubsan_handle_add_overflow() {
}
void __ubsan_handle_sub_overflow() {
}

void __ubsan_handle_type_mismatch_v1(struct type_mismatch_info *type_mismatch, uintptr_t pointer) {
        struct source_location *location = &type_mismatch->location;

        if (pointer == 0) {
                #ifdef UBSAN_NULL_PTR
                        dprintf("\n!!! UBSAN !!!\n");
                        dprintf("Null pointer access!\n");
                        print_location(location);
                #endif
        } else if (type_mismatch->alignment != 0 && is_aligned(pointer, type_mismatch->alignment)) {
                // Most useful on architectures with stricter memory alignment requirements, like ARM.
                #ifdef UBSAN_MEM_ALIGN
                        dprintf("\n!!! UBSAN !!!\n");
                        dprintf("Unaligned memory access!\n");
                        print_location(location);
                #endif
        } else {
                dprintf("\n!!! UBSAN !!!\n");
                dprintf("Insufficient size!\n");
                dprintf("%s address 0x%x with insufficient space for object of type %s\n",
                        Type_Check_Kinds[type_mismatch->type_check_kind], (void *)pointer,
                        type_mismatch->type->name);
                print_location(location);
        }

}
