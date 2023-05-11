include Makefile.inc

.PHONY: nconfig menuconfig initrd buildimg run-arm run-x64-efi run-x64-bios

nconfig:
	@ ./config/Configure ./config/config.in
	@ cp config/autoconf.h $(KERNDIR)/src/include/autoconf.h

menuconfig:
	@ ./config/Menuconfig ./config/config.in
	@ cp config/autoconf.h $(KERNDIR)/src/include/autoconf.h

initrd:
	@ tar -cvf initrd.tar module/*.elf

buildimg: initrd
	parted -s microk.img mklabel gpt
	parted -s microk.img mkpart ESP fat32 2048s 100%
	parted -s microk.img set 1 esp on
	./limine/limine-deploy microk.img
	sudo losetup -Pf --show microk.img
	sudo mkfs.fat -F 32 /dev/loop0p1
	mkdir -p img_mount
	sudo mount /dev/loop0p1 img_mount
	sudo mkdir -p img_mount/EFI/BOOT
	sudo cp -v $(KERNDIR)/microk.elf \
		   module/*.elf \
		   limine.cfg \
		   initrd.tar \
		   limine/limine.sys \
		   splash.ppm \
		   img_mount/
	sudo cp -v limine/BOOTX64.EFI img_mount/EFI/BOOT/
	sudo cp -v limine/BOOTAA64.EFI img_mount/EFI/BOOT/
	sync
	sudo umount img_mount
	sudo losetup -d /dev/loop0
	rm -rf img_mount

run-arm:
	qemu-system-aarch64 \
		-M hpet=on \
		-machine virt \
		-bios /usr/share/OVMF/aarch64/QEMU_CODE.fd  \
		-m 4G \
		-cpu cortex-a57 \
		-chardev stdio,id=char0,logfile=serial.log,signal=off \
		-serial chardev:char0 \
		-smp sockets=1,cores=4,threads=1 \
		-drive file="microk.img" \
		-s \
		-S \
		-device qemu-xhci
run-x64-bios:
	qemu-system-x86_64 \
		-M hpet=on \
		-m 4G \
		-chardev stdio,id=char0,logfile=serial.log,signal=off \
		-serial chardev:char0 \
		-smp sockets=1,cores=4,threads=1 \
		-drive file="microk.img" \
		-s \
		-machine type=q35 \
		-S \
		-device qemu-xhci

run-x64-efi:
	qemu-system-x86_64 \
		-bios /usr/share/OVMF/x64/OVMF_CODE.fd \
		-M hpet=on \
		-m 4G \
		-chardev stdio,id=char0,logfile=serial.log,signal=off \
		-serial chardev:char0 \
		-smp sockets=1,cores=4,threads=1 \
		-drive file="microk.img" \
		-s \
		-machine type=q35 \
		-S \
		-device qemu-xhci
