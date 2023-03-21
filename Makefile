ARCH = x86_64

KERNDIR = microk-kernel/src
MKMIDIR = mkmi/src

LDS64 = kernel64.ld
CC = $(ARCH)-elf-gcc
CPP = $(ARCH)-elf-g++
ASM = nasm
LD = $(ARCH)-elf-ld

CFLAGS = -ffreestanding             \
	 -fstack-protector          \
	 -fno-omit-frame-pointer    \
	 -fno-builtin-g             \
	 -I $(MKMIDIR)/include            \
	 -I $(KERNDIR)/include          \
	 -m64                       \
	 -mabi=sysv                 \
	 -mno-80387                 \
	 -mno-mmx                   \
	 -mno-sse                   \
	 -mno-sse2                  \
	 -mno-red-zone              \
	 -mcmodel=kernel            \
	 -fpermissive               \
	 -Wall                      \
	 -Wno-write-strings         \
	 -Og                        \
	 -fno-rtti                  \
	 -fno-exceptions            \
	 -fno-lto                   \
	 -fno-pie                   \
	 -fno-pic                   \
	 -march=x86-64              \
	 -ggdb


ASMFLAGS = -f elf64

LDFLAGS = -nostdlib                \
	  -static                  \
	  -m elf_$(ARCH)           \
	  -z max-page-size=0x1000

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

CPPSRC = $(call rwildcard,$(KERNDIR),*.cpp)
MKMISRC += $(call rwildcard,$(MKMIDIR),*.cpp)
ASMSRC = $(call rwildcard,$(KERNDIR),*.asm)
KOBJS = $(patsubst $(KERNDIR)/%.cpp, $(KERNDIR)/%.o, $(CPPSRC))
KOBJS += $(patsubst $(KERNDIR)/%.asm, $(KERNDIR)/%.o, $(ASMSRC))
KOBJS += $(patsubst $(MKMIDIR)/%.cpp, $(MKMIDIR)/%.o, $(MKMISRC))

kernel: $(KOBJS) link

nconfig:
	@ ./config/Configure ./config/config.in

menuconfig:
	@ ./config/Menuconfig ./config/config.in

$(MKMIDIR)/%.o: $(MKMIDIR)/%.cpp
	@ mkdir -p $(@D)
	@ echo !==== COMPILING $^ && \
	$(CPP) $(CFLAGS) -c $^ -o $@


$(KERNDIR)/%.o: $(KERNDIR)/%.cpp
	@ mkdir -p $(@D)
	@ echo !==== COMPILING $^ && \
	$(CPP) $(CFLAGS) -c $^ -o $@


$(KERNDIR)/%.o: $(KERNDIR)/%.asm
	@ mkdir -p $(@D)
	@ echo !==== COMPILING $^  && \
	$(ASM) $(ASMFLAGS) $^ -o $@

link: $(KOBJS)
	@ echo !==== LINKING
	$(LD) $(LDFLAGS) -T $(KERNDIR)/kernel.ld -o microk.elf $(KOBJS)
	@ ./symbolstocpp.sh
	$(CPP) $(CFLAGS) -c $(KERNDIR)/sys/symbolMap.cpp -o $(KERNDIR)/sys/symbolMap.o
	$(LD) $(LDFLAGS) -T $(KERNDIR)/kernel.ld -o microk.elf $(KOBJS)


clean:
	@rm $(KOBJS) $(MODOBJS)

initrd:
	@ tar -cvf initrd.tar module/*.elf

buildimg: initrd kernel
	parted -s microk.img mklabel gpt
	parted -s microk.img mkpart ESP fat32 2048s 100%
	parted -s microk.img set 1 esp on
	./limine/limine-deploy microk.img
	sudo losetup -Pf --show microk.img
	sudo mkfs.fat -F 32 /dev/loop0p1
	mkdir -p img_mount
	sudo mount /dev/loop0p1 img_mount
	sudo mkdir -p img_mount/EFI/BOOT
	sudo cp -v microk.elf \
		   limine.cfg \
		   initrd.tar \
		   limine/limine.sys \
		   img_mount/
	sudo cp -v limine/BOOTX64.EFI img_mount/EFI/BOOT/
	sudo cp -v limine/BOOTAA64.EFI img_mount/EFI/BOOT/
	sync
	sudo umount img_mount
	sudo losetup -d /dev/loop0
	rm -rf img_mount
run-arm:
	qemu-system-aarch64 \
		-machine virt \
		-bios /usr/share/OVMF/aarch64/QEMU_CODE.fd  \
		-m 8G \
		-cpu cortex-a57 \
		-chardev stdio,id=char0,logfile=serial.log,signal=off \
		-serial chardev:char0 \
		-smp sockets=1,cores=4,threads=1 \
		-drive file="microk.img" \
		-s \
		-S \
		-device qemu-xhci
run:
	qemu-system-x86_64 \
		-bios /usr/share/OVMF/x64/OVMF.fd \
		-m 8G \
		-chardev stdio,id=char0,logfile=serial.log,signal=off \
		-serial chardev:char0 \
		-smp sockets=1,cores=4,threads=1 \
		-drive file="microk.img" \
		-s \
		-machine type=q35 \
		-S \
		-device qemu-xhci
