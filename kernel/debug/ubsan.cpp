#include <stdint.h>
#include <stddef.h>
#include <debug/ubsan.hpp>
#include <sys/printk.hpp>

const char *TypeCheckKinds[] = {
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

static inline void PrintLocation(SourceLocation *location) {
	PRINTK::PrintK(" file: %s\r\n line: %d\r\n column: %d\r\n", location->file, location->line, location->column);
}

extern "C" void __ubsan_handle_missing_return() {
}

extern "C" void __ubsan_handle_divrem_overflow() {
}

extern "C" void __ubsan_handle_load_invalid_value() {
}

extern "C" void __ubsan_handle_shift_out_of_bounds() {
}

extern "C" void __ubsan_handle_builtin_unreachable() {
}

extern "C" void __ubsan_handle_mul_overflow() {
}

extern "C" void __ubsan_handle_negate_overflow() {
}

extern "C" void __ubsan_handle_out_of_bounds(void* dataRaw, void* indexRaw) {
        OutOfBoundsInfo *data = (OutOfBoundsInfo*)dataRaw;
        uintptr_t index = (uintptr_t)indexRaw;
        (void)index;

        PRINTK::PrintK("\r\n!!! UBSAN !!!\r\n"
                "Out of bounds!\r\n");

        PrintLocation(&data->location);
}

extern "C" void __ubsan_handle_pointer_overflow() {
}
extern "C" void __ubsan_handle_add_overflow() {
}
extern "C" void __ubsan_handle_sub_overflow() {
}

extern "C" void __ubsan_handle_type_mismatch_v1(TypeMismatchInfo *typeMismatch, uintptr_t pointer) {
        SourceLocation *location = &typeMismatch->location;

        if (pointer == 0) {
                PRINTK::PrintK("\r\n!!! UBSAN !!!\r\n");
                PRINTK::PrintK("Null pointer access!\r\n");
                PrintLocation(location);
        } else if (typeMismatch->alignment != 0 && IsAligned(pointer, typeMismatch->alignment)) {
                // Most useful on architectures with stricter memory alignment requirements, like ARM.
                PRINTK::PrintK("\r\n!!! UBSAN !!!\r\n");
                PRINTK::PrintK("Unaligned memory access!\r\n");
                PrintLocation(location);
        } else {
                PRINTK::PrintK("\r\n!!! UBSAN !!!\r\n");
                PRINTK::PrintK("Insufficient size!\r\n");
                PRINTK::PrintK("%s address 0x%x with insufficient space for object of type %s\r\n",
                        TypeCheckKinds[typeMismatch->typeCheckKind], (void *)pointer,
                        typeMismatch->type->name);
                PrintLocation(location);
        }

}
