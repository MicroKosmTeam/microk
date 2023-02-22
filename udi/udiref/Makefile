#
# File: top-of-tree/Makefile
#
# This gets the ball rolling when building the UDI source.
# Some top-level build commands that are useful are:
#    build   : The default rule. Build all the source.
#    install : Install binaries that were built and the headers.
#    clean   : Remove built components from your source tree.
#    all     : Not quite the same as build.
#    dbg     : Print build commands.
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

SHELL=/bin/sh
SUBDIRS=tools env meta mapper driver
OBJDIRS=env/.build meta/.build mapper/.build driver/.build
PUB_VERSION=1.01
PUBVDIRS= \
	pub_include/udi/$(PUB_VERSION) \
	pub_include/udi/$(PUB_VERSION)/udi_core \
	pub_include/udi_nic/$(PUB_VERSION) \
	pub_include/udi_pci/$(PUB_VERSION) \
	pub_include/udi_physio/$(PUB_VERSION) \
	pub_include/udi_scsi/$(PUB_VERSION)

build: FRC $(PUBVDIRS)
	TOPDIR=`pwd`; \
	ENVDIR=`pwd`; \
	UDI_PUB_INCLUDE=`pwd`/pub_include; \
	export TOPDIR ENVDIR UDI_PUB_INCLUDE; \
	for d in $(SUBDIRS) ; do \
	    (cd $$d && echo "--------Building in `pwd`" && \
	     INDIR=$$d \
	     $(MAKE) build) || exit 1 ;\
	done;

dbg: FRC
	TOPDIR=`pwd`; \
	ENVDIR=`pwd`; \
	UDI_PUB_INCLUDE=`pwd`/pub_include; \
	export TOPDIR ENVDIR UDI_PUB_INCLUDE; \
	for d in $(SUBDIRS) ; do \
	    (cd $$d && echo "--------Building in `pwd`" && INDIR=$$d \
	     $(MAKE) -pn dbg);\
	done;

dbg1: FRC
	TOPDIR=`pwd`; \
	ENVDIR=`pwd`; \
	UDI_PUB_INCLUDE=`pwd`/pub_include; \
	export TOPDIR ENVDIR UDI_PUB_INCLUDE; \
	for d in $(SUBDIRS) ; do \
	    (cd $$d && echo "--------Building in `pwd`" && INDIR=$$d \
	     $(MAKE) dbg); \
	done;

all: FRC
	TOPDIR=`pwd`; \
	ENVDIR=`pwd`; \
	UDI_PUB_INCLUDE=`pwd`/pub_include; \
	export TOPDIR ENVDIR UDI_PUB_INCLUDE; \
	for d in $(SUBDIRS) ; do \
	    (cd $$d && echo "--------Building ALL in `pwd`" && INDIR=$$d \
	     $(MAKE) all); \
	done

pub_include/udi/$(PUB_VERSION):
	rm -rf pub_include/udi/$(PUB_VERSION) 2>/dev/null
	mkdir -p pub_include/udi/$(PUB_VERSION)
	ln -sf ../../udi.h pub_include/udi/$(PUB_VERSION)/udi.h

pub_include/udi/$(PUB_VERSION)/udi_core:
	ln -sf ../../udi_core pub_include/udi/$(PUB_VERSION)/udi_core

pub_include/udi_nic/$(PUB_VERSION):
	rm -rf pub_include/udi_nic/$(PUB_VERSION) 2>/dev/null
	mkdir -p pub_include/udi_nic/$(PUB_VERSION)
	ln -sf ../../udi_nic.h pub_include/udi_nic/$(PUB_VERSION)/udi_nic.h

pub_include/udi_pci/$(PUB_VERSION):
	rm -rf pub_include/udi_pci/$(PUB_VERSION) 2>/dev/null
	mkdir -p pub_include/udi_pci/$(PUB_VERSION)
	ln -sf ../../udi_pci.h pub_include/udi_pci/$(PUB_VERSION)/udi_pci.h

pub_include/udi_physio/$(PUB_VERSION):
	rm -rf pub_include/udi_physio/$(PUB_VERSION) 2>/dev/null
	mkdir -p pub_include/udi_physio/$(PUB_VERSION)
	ln -sf ../../udi_physio.h pub_include/udi_physio/$(PUB_VERSION)/udi_physio.h
	ln -sf ../udi_phys/udi_bridge.h pub_include/udi_physio/udi_bridge.h
	ln -sf ../udi_phys/udi_dma.h pub_include/udi_physio/udi_dma.h
	ln -sf ../udi_phys/udi_pio.h pub_include/udi_physio/udi_pio.h

pub_include/udi_scsi/$(PUB_VERSION):
	rm -rf pub_include/udi_scsi/$(PUB_VERSION) 2>/dev/null
	mkdir -p pub_include/udi_scsi/$(PUB_VERSION)
	ln -sf ../../udi_scsi.h pub_include/udi_scsi/$(PUB_VERSION)/udi_scsi.h

install: FRC
	UDI_PUB_INCLUDE=`pwd`/pub_include; \
	TOPDIR=`pwd`; ENVDIR=`pwd`; export TOPDIR ENVDIR UDI_PUB_INCLUDE; \
	for d in $(SUBDIRS) ; do \
	    (cd $$d && echo "--------Installing in `pwd`" && INDIR=$$d \
	     $(MAKE) install || exit 1) || exit 1;\
	done

# The following cleans pub hdrs, .build dirs, udi-pkg.1 dirs, and then
# invokes subdirectory cleanup.  There are no comments embedded
# because some OS's can't handle that (e.g. Tru64)

clean:: FRC
	rm -rf pub_include/udi
	rm -rf pub_include/udi_nic
	rm -rf pub_include/udi_pci
	rm -rf pub_include/udi_physio
	rm -rf pub_include/udi_scsi
	@find pub_include -type l -exec rm {} \; 2>/dev/null || true 

	@find . -name ".build" -exec echo "--------Cleaning" {} \; -exec rm -rf {} \; 2> /dev/null || true
	@find . -name "udi-pkg.1" -exec echo "--------Cleaning" {} \; -exec rm -rf {} \; 2>/dev/null || true
	@ cd tools  && echo "--------Cleaning in tools"  && $(MAKE) clean
	@ cd meta   && echo "--------Cleaning in meta"   && $(MAKE) clean
	@ cd driver && echo "--------Cleaning in driver" && $(MAKE) clean
	@ cd mapper && echo "--------Cleaning in mapper" && $(MAKE) clean
	@ cd env    && echo "--------Cleaning in env"    && $(MAKE) clean

FRC:

cscope:
	( echo -p2 ; find `pwd` -name '*.[ch]' ) | egrep -v /\..*\(user_env\|POSIX\)/ > cscope.files 
