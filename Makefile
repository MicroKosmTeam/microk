bootloader_grub_source_files := $(shell find src/bootloader_grub -name *.S)
bootloader_grub_object_files := $(patsubst src/bootloader_grub/%.S, build/bootloader_grub/%.o, $(bootloader_grub_source_files))
bootloader_efi_source_files := $(shell find src/bootloader_efi -name *.c)
bootloader_efi_object_files := $(patsubst src/bootloader_efi/%.c, build/bootloader_efi/%.o, $(bootloader_efi_source_files))
kernel_c_source_files := $(shell find src/kernel -name *.c)
kernel_asm_source_files := $(shell find src/kernel -name *.S)
kernel_c_object_files := $(patsubst src/kernel/%.c, build/kernel/%.o, $(kernel_c_source_files))
kernel_asm_object_files := $(patsubst src/kernel/%.S, build/kernel/%.o, $(kernel_asm_source_files))

$(bootloader_grub_object_files): build/bootloader_grub/%.o : src/bootloader_grub/%.S
	@ echo Making bootloader object file $@...
	mkdir -p $(dir $@) && \
	x86_64-elf-gcc -mcmodel=large -c -I src/kernel/include -ffreestanding $(patsubst build/bootloader_grub/%.o, src/bootloader_grub/%.S, $@) -o $@

$(bootloader_efi_object_files): build/bootloader_efi/%.o : src/bootloader_efi/%.c
	@ echo Making bootloader object file $@...
	mkdir -p $(dir $@) && \
	gcc -I src/gnu-efi/inc -mcmodel=large -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c $(patsubst build/bootloader_efi/%.o, src/bootloader_efi/%.c, $@) -o $@

bootloader_efi: $(bootloader_efi_object_files)
	ld -shared -Bsymbolic -L src/gnu-efi/x86_64/lib -L src/gnu-efi/x86_64/gnuefi -T src/gnu-efi/gnuefi/elf_x86_64_efi.lds src/gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o $(bootloader_efi_object_files) -o build/bootloader_efi/loader.so -lgnuefi -lefi && \
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 build/bootloader_efi/loader.so build/bootloader_efi/loader.efi && \
	cp build/bootloader_efi/loader.efi dist/x86_64-efi/loader.efi

$(kernel_c_object_files): build/kernel/%.o : src/kernel/%.c
	@ echo Making kernel object file $@...
	mkdir -p $(dir $@) && \
	x86_64-elf-gcc -mcmodel=large -c -I src/kernel/include -ffreestanding $(patsubst build/kernel/%.o, src/kernel/%.c, $@) -o $@

$(kernel_asm_object_files): build/kernel/%.o : src/kernel/%.S
	@ echo Making kernel object file $@...
	mkdir -p $(dir $@) && \
	x86_64-elf-gcc -mcmodel=large -c -I src/kernel/include -ffreestanding $(patsubst build/kernel/%.o, src/kernel/%.S, $@) -o $@

run-bios:
	qemu-system-x86_64 -cdrom dist/x86_64-grub/kernel.iso -m 6G -accel kvm

run-efi:
	qemu-system-x86_64 -cdrom dist/x86_64-efi/microk.iso -m 6G -accel kvm -bios /usr/share/OVMF/x64/OVMF.fd

run-serial-bios:
	qemu-system-x86_64 -cdrom dist/x86_64-grub/kernel.iso -m 6G -accel kvm -nographic && tput reset

run-serial-efi:
	qemu-system-x86_64 -cdrom dist/x86_64-efi/microk.iso -m 6G -accel kvm -nographic -bios /usr/share/OVMF/x64/OVMF.fd && tput reset


clean:
	rm -rf build

.PHONY: build-x86_64-efi

build-x86_64-efi: bootloader_efi $(kernel_c_object_files) $(kernel_asm_object_files)
	mkdir -p dist/x86_64-efi && \
	x86_64-elf-gcc -nostdlib -lgcc -n -o dist/x86_64-efi/kernel.bin -T targets/x86_64-efi/linker.ld $(kernel_c_object_files) $(kernel_asm_object_files) && \
	cp dist/x86_64-efi/kernel.bin targets/x86_64-efi/img/kernel.elf && \
	cp dist/x86_64-efi/loader.efi targets/x86_64-efi/img/bootx64.efi && \
	dd if=/dev/zero of=targets/x86_64-efi/microk.img bs=512 count=93750 && \
	mkfs.fat -F 32 targets/x86_64-efi/microk.img && \
	mmd -i targets/x86_64-efi/microk.img ::/EFI && \
	mmd -i targets/x86_64-efi/microk.img ::/EFI/BOOT && \
	mcopy -i targets/x86_64-efi/microk.img targets/x86_64-efi/img/bootx64.efi ::/EFI/BOOT && \
	mcopy -i targets/x86_64-efi/microk.img targets/x86_64-efi/img/startup.nsh :: && \
	mcopy -i targets/x86_64-efi/microk.img targets/x86_64-efi/img/kernel.elf :: && \
	mcopy -i targets/x86_64-efi/microk.img targets/x86_64-efi/img/zap-light16.psf :: && \
	cp targets/x86_64-efi/microk.img dist/x86_64-efi/microk.img


build-x86_64-grub: $(bootloader_grub_object_files) $(kernel_c_object_files) $(kernel_asm_object_files)
	mkdir -p dist/x86_64-grub && \
	x86_64-elf-gcc -nostdlib -lgcc -n -o dist/x86_64-grub/kernel.bin -T targets/x86_64-grub/linker.ld $(bootloader_grub_object_files) $(kernel_c_object_files) $(kernel_asm_object_files) && \
	cp dist/x86_64-grub/kernel.bin targets/x86_64-grub/iso/boot/kernel.bin && \
	grub-mkrescue /usr/lib/grub/i386-pc -o dist/x86_64-grub/kernel.iso targets/x86_64-grub/iso
