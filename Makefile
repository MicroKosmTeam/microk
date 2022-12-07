kernel_c_source_files := $(shell find src/kernel -name *.c)
kernel_asm_source_files := $(shell find src/kernel -name *.S)
kernel_c_object_files := $(patsubst src/kernel/%.c, build/kernel/%.o, $(kernel_c_source_files))
kernel_asm_object_files := $(patsubst src/kernel/%.S, build/kernel/%.o, $(kernel_asm_source_files))

$(kernel_c_object_files): build/kernel/%.o : src/kernel/%.c
	mkdir -p $(dir $@) && \
	x86_64-elf-gcc -mcmodel=large -c -I src/kernel/include -ffreestanding $(patsubst build/kernel/%.o, src/kernel/%.c, $@) -o $@

$(kernel_asm_object_files): build/kernel/%.o : src/kernel/%.S
	mkdir -p $(dir $@) && \
	x86_64-elf-gcc -mcmodel=large -c -I src/kernel/include -ffreestanding $(patsubst build/kernel/%.o, src/kernel/%.S, $@) -o $@

run:
	qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso -m 6G -accel kvm

clean:
	rm -rf build

.PHONY: build-x86_64
build-x86_64: $(kernel_c_object_files) $(kernel_asm_object_files)
	mkdir -p dist/x86_64 && \
	x86_64-elf-gcc -nostdlib -lgcc -n -o dist/x86_64/kernel.bin -T targets/x86_64/linker.ld $(kernel_c_object_files) $(kernel_asm_object_files) && \
	cp dist/x86_64/kernel.bin targets/x86_64/iso/boot/kernel.bin && \
	grub-mkrescue /usr/lib/grub/i386-pc -o dist/x86_64/kernel.iso targets/x86_64/iso
