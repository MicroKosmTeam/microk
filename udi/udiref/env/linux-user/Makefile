#
# File: linux-user/Makefile
#
# Makefile for the UDI Prototype Development Effort: Linux User Environment
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
# Abstract: The goal here is to build the UDI User-space Environment for
#           Linux.
##############################################################################

SHELL=/bin/sh
TOPDIR=../..
OBJDIR=../.build
META_SRCDIR=../../meta
INDIR=env/linux-user


# This is a makefile which is invoked from the primary makefile after that
# makefile sets the appropriate environment variables (via including the
# env/uw/Make_conf.uw file) before invoking this make with the
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


build: mkpop envcommon osdep_pops.h mgmtmeta ma_msgs.txt

mkpop: ../common/mkpop.c pio_pops.h
	$(CC) -g -Wall -I. -o ./mkpop ../common/mkpop.c

envcommon:
	ln -s ../common ./envcommon

ma_msgs.txt: envcommon/ma_msgs.txt
	ln -s envcommon/ma_msgs.txt ma_msgs.txt

mgmtmeta:
	ln -s $(META_SRCDIR)/mgmt ./mgmtmeta

osdep_pops.h: mkpop pio_pops.h
	./mkpop > osdep_pops.h

all: real-build

install:

clean:
	rm -rf mkpop osdep_pops.h bin udiget

