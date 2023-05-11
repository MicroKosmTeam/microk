ARCH = x86_64

CC = $(ARCH)-elf-gcc
CPP = $(ARCH)-elf-g++
LD = $(ARCH)-elf-ld
AR = $(ARCH)-elf-ar
ASM = nasm

KERNDIR = microk-kernel
MKMIDIR = mkmi

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
