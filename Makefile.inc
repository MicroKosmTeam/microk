ARCH = x86_64
COMPILER_PATH=$(PWD)/compiler/target/$(ARCH)-elf/bin

CC = $(COMPILER_PATH)/$(ARCH)-elf-gcc
CPP = $(COMPILER_PATH)/$(ARCH)-elf-g++
LD = $(COMPILER_PATH)/$(ARCH)-elf-ld
AR = $(COMPILER_PATH)/$(ARCH)-elf-ar
AS = $(COMPILER_PATH)/$(ARCH)-elf-as

ASM = nasm
#ASM = $(AS)

KERNDIR = microk-kernel
MKMIDIR = mkmi

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
