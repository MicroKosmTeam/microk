OSNAME = microk
ARCH = x86_64

SRCDIR = src
OBJDIR = obj
BINDIR = bin

KERNDIR = $(SRCDIR)/kernel
EFIDIR = $(SRCDIR)/gnu-efi
MODDIR = $(SRCDIR)/module
BOOTEFI := $(EFIDIR)/x86_64/bootloader/main.efi

OVMFDIR = OVMFbin
LDS64 = kernel64.ld
CC = $(ARCH)-elf-gcc
CPP = $(ARCH)-elf-g++
ASMC = nasm
LD = $(ARCH)-elf-gcc

CFLAGS = -g \
	 -mcmodel=kernel \
	 -mabi=sysv      \
	 -fno-builtin-g  \
	 -ffreestanding  \
	 -fshort-wchar   \
	 -mno-red-zone   \
	 -fno-omit-frame-pointer \
	 -Wall           \
	 -I src/kernel/include \
	 -fno-exceptions \
	 -fno-rtti       \
	 -std=c++17      \
	 -fstack-protector-all \
	 -fsanitize=undefined \
	 -fno-strict-aliasing \
	 -fms-extensions \
	 -mno-soft-float \
	 -mno-80387 \
	 -mno-mmx \
	 -mno-sse \
	 -mno-sse2 \
	 -fpermissive \
	 -Og
ASMFLAGS = -f elf64
LDFLAGS = -T $(LDS64) -z max-page-size=0x1000 -static -Bsymbolic -nostdlib

QEMUFLAGS = -drive file=$(BINDIR)/$(OSNAME).img,if=none,id=nvm -device nvme,serial=deadbeef,drive=nvm -m 8G -cpu qemu64 -smp sockets=1,cores=4,threads=1 -drive if=pflash,format=raw,unit=0,file="$(OVMFDIR)/OVMF_CODE-pure-efi.fd",readonly=on -drive if=pflash,format=raw,unit=1,file="$(OVMFDIR)/OVMF_VARS-pure-efi.fd" -device qemu-xhci -no-reboot -no-shutdown -M q35# -s -S  #,accel=tcg

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

KSRC = $(call rwildcard,$(KERNDIR),*.cpp)
KASMSRC = $(call rwildcard,$(KERNDIR),*.asm)
KCSRC = $(call rwildcard,$(KERNDIR),*.c)
MSRC =  $(call rwildcard,$(MODDIR),*.cpp)
KOBJS = $(patsubst $(KERNDIR)/%.cpp, $(OBJDIR)/kernel/%.o, $(KSRC))
KOBJS += $(patsubst $(KERNDIR)/%.c, $(OBJDIR)/kernel/%_c.o, $(KCSRC))
KOBJS += $(patsubst $(KERNDIR)/%.asm, $(OBJDIR)/kernel/%_asm.o, $(KASMSRC))
MOBJS = $(patsubst $(MODDIR)/%.cpp, $(OBJDIR)/module/%.o, $(MSRC))
DIRS = $(wildcard $(SRCDIR)/*)

kernel: setup bootloader $(KOBJS) link $(MOBJS)

$(OBJDIR)/kernel/arch/x86_64/interrupts/interrupts.o: $(KERNDIR)/arch/x86_64/interrupts/interrupts.cpp
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(CPP) -mgeneral-regs-only $(CFLAGS) -c $^ -o $@

$(OBJDIR)/kernel/%.o: $(KERNDIR)/%.cpp
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(CPP) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/kernel/%_c.o: $(KERNDIR)/%.c
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/kernel/%_asm.o: $(KERNDIR)/%.asm
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(ASMC) $(ASMFLAGS) $^ -o $@

$(OBJDIR)/module/%.o: $(MODDIR)/%.cpp
	@ echo !==== COMPILING MODULE $^
	@ mkdir -p $(@D)
	$(CPP) $(CFLAGS) -c $^ -o $@

config:
	@ ./src/config/Configure ./src/config/config.in

menuconfig:
	@ ./src/config/Menuconfig ./src/config/config.in

bootloader:
	make -C $(EFIDIR)
	make -C $(EFIDIR) bootloader

link: $(KOBJS)
	@ echo !==== LINKING
	$(LD) $(LDFLAGS) -o $(BINDIR)/kernel.elf $(KOBJS)
	bash ./symbolstocpp.sh
	@ echo !==== LINKING
	$(LD) $(LDFLAGS) -o $(BINDIR)/kernel.elf $(KOBJS)
	#strip -s $(BINDIR)/kernel.elf

setup:
	@mkdir -p $(BINDIR)
	@mkdir -p $(SRCDIR)
	@mkdir -p $(OBJDIR)

clean:
	@rm -rf $(OBJDIR)

buildimg: kernel
	#dd if=/dev/zero of=$(BINDIR)/$(OSNAME).img bs=512 count=250000
	parted -s $(BINDIR)/$(OSNAME).img mklabel gpt
	parted -s $(BINDIR)/$(OSNAME).img mkpart ESP fat32 2048s 100%
	parted -s $(BINDIR)/$(OSNAME).img set 1 esp on
	./src/limine/limine-deploy $(BINDIR)/$(OSNAME).img
	sudo losetup -Pf --show $(BINDIR)/$(OSNAME).img
	sudo mkfs.fat -F 32 /dev/loop0p1
	mkdir -p img_mount
	sudo mount /dev/loop0p1 img_mount
	sudo mkdir -p img_mount/EFI/BOOT
	sudo cp -v $(BINDIR)/initrd.tar \
		   startup.nsh \
		   $(BINDIR)/kernel.elf \
		   $(BINDIR)/zap-light16.psf \
		   limine.cfg \
		   src/limine/limine.sys \
		   img_mount/
	sudo cp -v src/limine/BOOTX64.EFI img_mount/EFI/BOOT/
	sync
	sudo umount img_mount
	sudo losetup -d /dev/loop0
	rm -rf img_mount

run:
	qemu-system-x86_64 $(QEMUFLAGS) -serial file:serial.log -device virtio-gpu-pci
run-tty:
	qemu-system-x86_64 $(QEMUFLAGS) -nographic
