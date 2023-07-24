#!/bin/sh

make -C mkmi static && \
#make -C module/pci static && \
#make -C module/ext static && \
#make -C module/ahci static && \
make -C module/acpi static && \
make -C module/user static && \
make -C microk-kernel kernel && \
make buildimg && \
make run-x64-efi

