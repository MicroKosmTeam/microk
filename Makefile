include Makefile.inc

.PHONY: clean nconfig menuconfig initrd buildimg run-arm run-x64-efi run-x64-bios

compiler:
	@ cd ./compiler/
	@ ./compile.sh x86_64-elf
	@ cd ..

nconfig:
	@ ./config/Configure ./config/config.in
	@ cp config/autoconf.h $(KERNDIR)/src/include/autoconf.h

menuconfig:
	@ ./config/Menuconfig ./config/config.in
	@ cp config/autoconf.h $(KERNDIR)/src/include/autoconf.h

initrd:
	@ cp module/*.kmd base/modules
	@ cd base && tar -cvf ../initrd.tar * && cd ..

LOOPDEV=$(shell losetup -f)

buildimg: initrd
	parted -s microk.img mklabel gpt
	parted -s microk.img mkpart ESP fat32 2048s 100%
	parted -s microk.img set 1 esp on
	sudo losetup -Pf --show microk.img
	sudo mkfs.fat -F 32 $(LOOPDEV)p1
	mkdir -p img_mount
	sudo mount $(LOOPDEV)p1 img_mount
	sudo mkdir -p img_mount/EFI/BOOT
	sudo cp -v $(KERNDIR)/microk.elf \
		   limine/limine-bios.sys \
		   limine.cfg \
		   module/*.kmd \
		   initrd.tar \
		   img_mount/
	sudo cp module/manager.kmd img_mount/manager.kmd
	sudo cp -v limine/*.EFI img_mount/EFI/BOOT/
	sync
	sudo umount img_mount
	sudo losetup -d $(LOOPDEV)
	./limine/limine bios-install microk.img
	rm -rf img_mount

GENERIC_OUTPUT_OPTS := -chardev stdio,id=char0,logfile=serial.log,signal=off \
		       -serial chardev:char0 \
		       -monitor telnet::45454,server,nowait \
		       -display none


GENERIC_CPU_OPTS := -smp sockets=2,cores=2,threads=2 \
		    -object memory-backend-ram,size=4G,id=m0 \
		    -object memory-backend-ram,size=4G,id=m1 \
		    -object memory-backend-ram,size=4G,id=m2 \
		    -object memory-backend-ram,size=4G,id=m3 \
		    -numa node,memdev=m0,cpus=0-1,nodeid=0 \
		    -numa node,memdev=m1,cpus=2-3,nodeid=1 \
		    -numa node,memdev=m2,cpus=4-5,nodeid=2 \
		    -numa node,memdev=m3,cpus=6-7,nodeid=3

GENERIC_DEV_OPTS := -device qemu-xhci \
		    -net nic,model=virtio \
		    -device virtio-blk-pci,drive=drive0 \
		    -drive id=drive0,if=none,file="microk.img" \
		    -device ich9-intel-hda \
		    -device hda-micro \
		    -device usb-mouse

X86_OUTPUT_OPTS := $(GENERIC_OUTPUT_OPTS) \
		  -debugcon file:debug.log


run-aarch64:
	qemu-system-aarch64 \
		-bios EDK2Firmware/aarch64/QEMU_EFI.fd \
		-m 16G \
		$(GENERIC_OUTPUT_OPTS) \
		-cpu cortex-a57 \
		$(GENERIC_CPU_OPTS) \
		-machine virt \
		$(GENERIC_DEV_OPTS) \
		&
	sleep 1
	telnet localhost 45454

run-aarch64-debug:
	qemu-system-aarch64 \
		-bios EDK2Firmware/aarch64/QEMU_EFI.fd \
		-m 16G \
		$(GENERIC_OUTPUT_OPTS) \
		-cpu cortex-a57 \
		$(GENERIC_CPU_OPTS) \
		-machine virt \
		$(GENERIC_DEV_OPTS) \
		-S \
		-s \
		&
	sleep 1
	telnet localhost 45454

run-x64-bios:
	qemu-system-x86_64 \
		-M hpet=on \
		-m 16G \
		-accel kvm \
		$(X86_OUTPUT_OPTS) \
		-cpu host \
		$(GENERIC_CPU_OPTS) \
		-machine type=q35 \
		$(GENERIC_DEV_OPTS) \
		&
	sleep 1
	telnet localhost 45454

run-x64-efi:
	qemu-system-x86_64 \
		-bios ./EDK2Firmware/ovmf-x64/OVMF_CODE-pure-efi.fd \
		-M hpet=on \
		-m 16G \
		-accel kvm \
		$(X86_OUTPUT_OPTS) \
		-cpu host \
		$(GENERIC_CPU_OPTS) \
		-machine type=q35 \
		$(GENERIC_DEV_OPTS) \
		&
	sleep 1
	telnet localhost 45454


run-x64-debug-bios:
	qemu-system-x86_64 \
		-M hpet=on \
		-m 16G \
		-accel tcg \
		$(X86_OUTPUT_OPTS) \
		$(GENERIC_CPU_OPTS) \
		-machine type=q35 \
		$(GENERIC_DEV_OPTS) \
		-S \
		-s \
		&
	sleep 1
	telnet localhost 45454


run-x64-efi-debug:
	qemu-system-x86_64 \
		-bios ./EDK2Firmware/ovmf-x64/OVMF_CODE-pure-efi.fd \
		-M hpet=on \
		-m 16G \
		-accel tcg \
		$(X86_OUTPUT_OPTS) \
		-cpu host \
		$(GENERIC_CPU_OPTS) \
		-machine type=q35 \
		$(GENERIC_DEV_OPTS) \
		-s \
		-S \
		&
	sleep 1
	telnet localhost 45454


clean:
	./clean.sh
