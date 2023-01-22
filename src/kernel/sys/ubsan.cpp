#include <stdint.h>
#include <stddef.h>
#include <sys/panik.h>
#include <sys/printk.h>



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
#ifdef __cplusplus
extern "C" {
#endif
static void printk_location(struct source_location *location) {
    printk(" file: %s\n line: %d\n column: %d\n",
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
        #ifdef UBSAN_OOB
                out_of_bounds_info *data = (out_of_bounds_info*)data_raw;
                uintptr_t index = (uintptr_t)index_raw;
                (void)index;

                printk("\n!!! UBSAN !!!\n"
                       "Out of bounds!\n");

                printk_location(&data->location);
        #endif
 
        #ifdef UBSAN_PANIK
                PANIK("UBSAN Panicked.");
        #endif
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
                        printk("Null pointer access!\n");
                        printk_location(location);
                #endif
        } else if (type_mismatch->alignment != 0 && is_aligned(pointer, type_mismatch->alignment)) {
                // Most useful on architectures with stricter memory alignment requirements, like ARM.
                #ifdef UBSAN_MEM_ALIGN
                        printk("\n!!! UBSAN !!!\n");
                        printk("Unaligned memory access!\n"); 
                        printk_location(location);
                #endif
        } else {
                printk("\n!!! UBSAN !!!\n");
                printk("Insufficient size!\n");
                printk("%s address 0x%x with insufficient space for object of type %s\n",
                        Type_Check_Kinds[type_mismatch->type_check_kind], (void *)pointer,
                        type_mismatch->type->name);
                printk_location(location);
        }

        #ifdef UBSAN_PANIK
                PANIK("UBSAN Panicked.");
        #endif
}
#ifdef __cplusplus
}
#endif
