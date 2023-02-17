#
# File: env/linux/Makefile
#
# Makefile for the UDI Prototype Development Effort: Linux Environment
#
# Make arguments:
#	clean: Remove all generated files.
#	build: Create the opaque data headers and other generated files.
#		If you want to rebuild the environment UDI module, 
#		execute the command: (cd ..; make).
#	install: Installs non-UDI modules (env_test, mod_test).
#	nuke: Remove installed parts of UDI.
#

#
# $Copyright udi_reference:
# 
# 
#    Copyright (c) 1995-2001; Compaq Computer Corporation; Hewlett-Packard
#    Company; Interphase Corporation; The Santa Cruz Operation, Inc;
#    Software Technologies Group, Inc; and Sun Microsystems, Inc
#    (collectively, the "Copyright Holders").  All rights reserved.
# 
#    Redistribution and use in source and binary forms, with or without
#    modification, are permitted provided that the conditions are met:
# 
#            Redistributions of source code must retain the above
#            copyright notice, this list of conditions and the following
#            disclaimer.
# 
#            Redistributions in binary form must reproduce the above
#            copyright notice, this list of conditions and the following
#            disclaimers in the documentation and/or other materials
#            provided with the distribution.
# 
#            Neither the name of Project UDI nor the names of its
#            contributors may be used to endorse or promote products
#            derived from this software without specific prior written
#            permission.
# 
#    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#    "AS IS," AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#    HOLDERS OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
#    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
#    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
#    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
#    DAMAGE.
# 
#    THIS SOFTWARE IS BASED ON SOURCE CODE PROVIDED AS A SAMPLE REFERENCE
#    IMPLEMENTATION FOR VERSION 1.01 OF THE UDI CORE SPECIFICATION AND/OR
#    RELATED UDI SPECIFICATIONS. USE OF THIS SOFTWARE DOES NOT IN AND OF
#    ITSELF CONSTITUTE CONFORMANCE WITH THIS OR ANY OTHER VERSION OF ANY
#    UDI SPECIFICATION.
# 
# 
# $
#

##############################################################################
# Abstract: The goal here is to build the UDI Environment for Linux into a
#	loadable module called "udi_MA.o" that can be dynamically loaded. This
#	file is generated from the common environment code, which is linked
#	into env-common-Linux_2.udi, and the Linux-specific portion, which is
#	linked into env-Linux_2.udi. These two files are then linked together
#	to get udi_MA.o.
#
#	The entry point for the loadable module is in udi_env_init.c/.o. This
#	file is part of the env-Linux_2.udi file.
##############################################################################

ABI=$(shell sh ../../env/common/Make_conf -A)

SHELL=/bin/sh
TOPDIR=../..
OBJDIR=./bin/$(ABI)
INDIR=env/linux


# This is a makefile which is invoked from the primary makefile after that
# makefile sets the appropriate environment variables (via including the
# env/linux/Make_conf.linux file) before invoking this make with the
# build target:
#    INDIR  = (TOPDIR relative) directory to do actual make in when configured
#    TOPDIR = relation of INDIR to the top of the source tree
#    OBJDIR = (INDIR relative) directory to build objects in
#    OS_ENV = OS/architecture unique identifier
#    OS_NAME = OS short name (matches directory qualifiers)
#    CC = name of compiler
#    CFLAGS, ENVCFLAGS = flags to pass to compiler
#    CINCL, ENVCINCL = include directives for compiler
#    PUB_DIR = pub_include directory (absolute path name)
#    PUB_HDRS = list of env services & Mgmt Meta pub_include files
#    ENV_DIR = env directory (absolute path name)
#    ENV_HDRS = list of env header files needed by env modules & mappers
#    SUPP_BUSES = supported buses (as a list of bus binding header files)

LOCALCFLAGS=

#TARGETCC=$(shell TOPDIR=$(TOPDIR) sh ../../env/common/Make_conf -targetcc)
TARGETCC=$(shell TOPDIR=$(TOPDIR) sh getarchflags -targetcc)
OBJDUMP=objdump
TARGETKERNELCFLAGS=$(shell TOPDIR=$(TOPDIR) sh getarchflags -targetccopts) -D__NO_VERSION__


##############################################################################
# Build the Linux-specific portion of the UDI environment into a .udi file.
# Combine the common and Linux-specific portions of the UDI environment into
# the udi_env.o loadable module.
# Build the UDI environment unit test loadable module.
##############################################################################

build: mkpop udi_osdep_opaquetypes.h envcommon osdep_pops.h mgmtmeta ma_msgs.txt \
	opaquetest.out env_test.o mod_test.o

all: build

# env_test and mod_test should both have -O2 so any inlines used just work.
env_test.o: env_test.c
	$(TARGETCC) -O2 -g $(TARGETKERNELCFLAGS) -D_UDI_PHYSIO_SUPPORTED=1 -c env_test.c -o env_test.o -I. -I./envcommon -I$(TOPDIR)/pub_include

mod_test.o: mod_test.c
	$(TARGETCC) -O2 -g $(TARGETKERNELCFLAGS) -c mod_test.c -o mod_test.o -I. -I./envcommon -I$(TOPDIR)/pub_include

mkpop: ../common/mkpop.c pio_pops.h
	$(CC) -I. -o ./mkpop ../common/mkpop.c

mkopaque_mod.o: mkopaque_mod.c
	$(TARGETCC) $(TARGETKERNELCFLAGS) -I. -I./envcommmon -I$(TOPDIR)/pub_include -c -o mkopaque_mod.o mkopaque_mod.c

mkopaque.o: mkopaque.c
	$(CC) -c -I. -o mkopaque.o mkopaque.c

mkopaque: mkopaque.o mkopaque_mod.o
	$(CC) -o mkopaque mkopaque.o mkopaque_mod.o

opaquedefs: opaquedefs.c opaquetest.c envcommon mkopaque udi_osdep_opaquetypes.h
	$(TARGETCC) $(TARGETKERNELCFLAGS) -I. -I./envcommon -I$(TOPDIR)/pub_include -o ./opaquedefs opaquedefs.c

opaquetypes: opaquetypes.c opaquetest.c envcommon mkopaque udi_osdep_opaquetypes.h
	$(TARGETCC) $(TARGETKERNELCFLAGS) -I. -I./envcommon -I$(TOPDIR)/pub_include -o ./opaquetypes opaquetypes.c

opaquetest.out: opaquedefs opaquetypes
	./opaquedefs > /tmp/opaquedefs.out
	./opaquetypes > /tmp/opaquetypes.out
	@rm -rf ./opaquetest.out
	@diff /tmp/opaquedefs.out /tmp/opaquetypes.out > ./opaquetest.out
	@if [ -s ./opaquetest.out ]; then \
		echo "ERROR: opaque defintions do not match the generated opaque types."; \
		exit 2; \
	fi

udi_osdep_opaquetypes.h: mkopaque
	./mkopaque

# If this rule happened because opaquedefs changed, it is very likely that
# the whole environment and mappers must be recompiled due to data structure
# size changes in opaquedefs that would cause differences with the opaquetypes
# sizes.
#mkopaque-bin.o: mkopaque-bin.c udi_osdep_opaquedefs.h
#	$(TARGETCC) $(TARGETKERNELCFLAGS) -I. -I../../pub_include -c -o mkopaque-bin.o mkopaque-bin.c
#
#udi_osdep_opaquetypes.h: mkopaque-bin.o
#	@echo "/*" > udi_osdep_opaquetypes.h
#	@echo " * This file was machine-generated." >> udi_osdep_opaquetypes.h
#	@echo " * Editing it is almost certainly a mistake." >> udi_osdep_opaquetypes.h
#	@echo " */" >> udi_osdep_opaquetypes.h
#	@echo "" >> udi_osdep_opaquetypes.h
#	@echo "#ifndef __UDI_OSDEP_OPAQUETYPES_H__" >> udi_osdep_opaquetypes.h
#	@echo "#       define __UDI_OSDEP_OPAQUETYPES_H__" >> udi_osdep_opaquetypes.h
#	@echo "" >> udi_osdep_opaquetypes.h
#	@echo "" >> udi_osdep_opaquetypes.h
#	@$(OBJDUMP) -t mkopaque-bin.o | grep _opaque | grep -v var | grep -v IS_INVALID |grep -v is_invalid | awk \
#'{print "struct " $$5 "  {\n \
#unsigned long TOP_PAD[0];\n \
#char opaque[ 0x" $$1 " ];\n \
#unsigned long END_PAD[0];\n \
#};\n " }'   >> udi_osdep_opaquetypes.h
#	@echo "" >> udi_osdep_opaquetypes.h
#	@echo "" >> udi_osdep_opaquetypes.h
#	@echo "#endif  /* __UDI_OSDEP_OPAQUETYPES_H__ */" >> udi_osdep_opaquetypes.h
#	@echo "" >> udi_osdep_opaquetypes.h
#

# Automatically check that the first number of each line in opaquetest.out is zero.
#opaquetest.out: mkopaque-bin.o
#	@$(OBJDUMP) -t mkopaque-bin.o | grep IS_INVALID > opaquetest.out
#	@awk '{ if ($$1 != 00000000) { print "Problem, data structure " $$5 "."; exit 1;} }' < opaquetest.out

envcommon:
	ln -s ../common ./envcommon

ma_msgs.txt: envcommon/ma_msgs.txt
	ln -s envcommon/ma_msgs.txt ma_msgs.txt

mgmtmeta:
	ln -s ../../meta/mgmt ./mgmtmeta

osdep_pops.h: mkpop pio_pops.h
	./mkpop > osdep_pops.h

CONFDIR=/etc/conf
MODULESPATH=/lib/modules

# Get the kernel version
KVER=$(shell if [ "X$(LINUX_KERNEL_SOURCE_BASE)" = "X" ]; then \
	cc -nostdinc -I$(MODULESPATH)/`uname -r`/build/include -E -dM kernver.c| awk -F\" '/RELEASE/ {print $$2}'; \
else \
	cc -nostdinc -I$(LINUX_KERNEL_SOURCE_BASE)/include -E -dM kernver.c| awk -F\" '/RELEASE/ {print$$2}'; \
	fi )


install: build
	/usr/bin/install -d $(MODULESPATH)/$(KVER)/misc;
	/usr/bin/install -c ./env_test.o $(MODULESPATH)/$(KVER)/misc;
	/usr/bin/install -c ./mod_test.o $(MODULESPATH)/$(KVER)/misc;
	/sbin/depmod $(MODULESPATH)/$(KVER)/misc/env_test.o;
	/sbin/depmod $(MODULESPATH)/$(KVER)/misc/mod_test.o; 
#	/sbin/depmod -a > /dev/null

clean:
	rm -f ./mkopaque ./mkpop ./osdep_pops.h ./udi_osdep_opaquetypes.h;
	rm -f ./opaquedefs ./opaquetypes ./opaquetest.out;
	rm -f ./*.o;

nuke:
	rm -f   $(MODULESPATH)/$(KVER)/misc/udi_MA.o \
		$(MODULESPATH)/$(KVER)/misc/udi_bridge.o \
		$(MODULESPATH)/$(KVER)/misc/udi_gio.o \
		$(MODULESPATH)/$(KVER)/misc/udi_nic.o \
		$(MODULESPATH)/$(KVER)/misc/udi_scsi.o \
		$(MODULESPATH)/$(KVER)/misc/udiM_bridge.o \
		$(MODULESPATH)/$(KVER)/misc/udiM_gio.o \
		$(MODULESPATH)/$(KVER)/misc/udiM_nic.o \
		$(MODULESPATH)/$(KVER)/misc/udiM_scsi.o \
		$(MODULESPATH)/$(KVER)/misc/udi_cmos.o \
		$(MODULESPATH)/$(KVER)/misc/udi_dpt.o \
		$(MODULESPATH)/$(KVER)/misc/udi_gpig.o \
		$(MODULESPATH)/$(KVER)/misc/shrkudi.o \
		$(MODULESPATH)/$(KVER)/misc/pseudod.o \
		$(MODULESPATH)/$(KVER)/misc/pseudond.o

