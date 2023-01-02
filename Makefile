kernel_c_source_files := $(shell find src/kernel -name *.c)
kernel_asm_source_files := $(shell find src/kernel -name *.S)
kernel_c_object_files := $(patsubst src/kernel/%.c, build/kernel/%.o, $(kernel_c_source_files))
kernel_asm_object_files := $(patsubst src/kernel/%.S, build/kernel/%.o, $(kernel_asm_source_files))

CC64 = x86_64-elf-gcc
CFLAGS = -static -g -mcmodel=large -c -I src/kernel/include -ffreestanding -fno-stack-protector -mno-red-zone

bootloader_efi:
	make -C src/gnu-efi
	make -C src/gnu-efi bootloader
	mv src/gnu-efi/x86_64/bootloader/main.efi dist/x86_64-efi/loader.efi

$(kernel_c_object_files): build/kernel/%.o : src/kernel/%.c
	@ echo Making kernel object file $@...
	mkdir -p $(dir $@) && \
	$(CC64) $(CFLAGS) $(patsubst build/kernel/%.o, src/kernel/%.c, $@) -o $@

$(kernel_asm_object_files): build/kernel/%.o : src/kernel/%.S
	@ echo Making kernel object file $@...
	mkdir -p $(dir $@) && \
	$(CC64) $(CFLAGS) $(patsubst build/kernel/%.o, src/kernel/%.S, $@) -o $@

run-bios:
	qemu-system-x86_64 -cdrom dist/x86_64-grub/kernel.iso -m 8G -accel kvm

run-efi:
	qemu-system-x86_64 -drive file=dist/x86_64-efi/microk.img -bios /usr/share/OVMF/x64/OVMF.fd -m 8G -accel kvm 

run-serial-bios:
	qemu-system-x86_64 -cdrom dist/x86_64-grub/kernel.iso -m 8G -accel kvm -nographic && tput reset

run-serial-efi:
	qemu-system-x86_64 -drive file=dist/x86_64-efi/microk.img -m 8G -accel kvm -nographic -bios /usr/share/OVMF/x64/OVMF.fd && tput reset


clean:
	rm -rf build

.PHONY: build-x86_64-efi

build-x86_64-efi: bootloader_efi $(kernel_c_object_files) $(kernel_asm_object_files)
	mkdir -p dist/x86_64-efi && \
	$(CC64) -nostdlib -lgcc -n -o dist/x86_64-efi/kernel.bin -T targets/x86_64-efi/linker.ld $(kernel_c_object_files) $(kernel_asm_object_files) && \
	cp dist/x86_64-efi/kernel.bin targets/x86_64-efi/img/kernel.elf && \
	cp dist/x86_64-efi/loader.efi targets/x86_64-efi/img/bootx64.efi && \
	dd if=/dev/zero of=targets/x86_64-efi/microk.img bs=512 count=93750 && \
	mformat -i targets/x86_64-efi/microk.img && \
	mmd -i targets/x86_64-efi/microk.img ::/EFI && \
	mmd -i targets/x86_64-efi/microk.img ::/EFI/BOOT && \
	mcopy -i targets/x86_64-efi/microk.img targets/x86_64-efi/img/bootx64.efi ::/EFI/BOOT && \
	mcopy -i targets/x86_64-efi/microk.img targets/x86_64-efi/img/startup.nsh :: && \
	mcopy -i targets/x86_64-efi/microk.img targets/x86_64-efi/img/kernel.elf :: && \
	mcopy -i targets/x86_64-efi/microk.img targets/x86_64-efi/img/zap-light16.psf :: && \
	cp targets/x86_64-efi/microk.img dist/x86_64-efi/microk.img
