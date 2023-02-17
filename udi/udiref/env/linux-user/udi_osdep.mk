#
# File: env/linux-user/udi_osdep.mk
#
# A file included by ../posix/Makefile.
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


#
# When cross-compiling, make sure you choose target-configured
# versions of libelf and efence.
#

#
# On RedHat 7, enable the next line. gcc 2.96 with RedHat is a
# broken beta version of gcc.
#
#CC=kgcc


#
# libelf configuration
#
LIBELFINC=-I/usr/local/include
LIBELFLIB=-L/usr/local/lib -lelf


#
# Link against no malloc debugger library.
#
#MALLOCDEBUGLIB=

#
# Link against Electric Fence.  Redhat ships with this, but it's available
# from rpmfind.net if you don't have it.
#
MALLOCDEBUGLIB=-L/usr/lib -lefence

#
# Link against njamd (Not Just Another Memory Debugger). See
# freshmeat.net.
#
#MALLOCDEBUGLIB=-L/usr/local/lib -lnjamd

#
# Architecture independent flags
#
ARCHINDEP_FLAGS= -fsigned-char -fno-omit-frame-pointer -D_POSIX_THREAD_SAFE_FUNCTIONS -D_POSIX_THREADS -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE

#
# Miscellaneous compilation flags appended to udibuild't things.
#
FLAGS=-pthread  -Wall $(LIBELFINC) $(ARCHINDEP_FLAGS)


# For DBG, you can specify these options:
#   -g  : normal quanity of debug symbols
#   -g3 : enormous amounts of debug symbols
#   -DDEBUG : to enable some linux-user debugging
#   -DUDI_DEBUG : to enable some udi-environment debugging
#   -O -O2 -O3 : optimization levels
DBG=-g -O2
# -DUDI_DEBUG


# No idea how to describe how this is used....
# For DEFS, you can specify these options:
#   -D_GNU_SOURCE -D_POSIX_SEMAPHORES : use posix semaphores for atomic ints.
#                       : (Default is to use pthread mutex based atomic ints.)
#   -DSTATIC : disable the default -DSTATIC=static.
#   -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE : enable latest version of libc.
#   -D_POSIX_THREAD_SAFE_FUNCTIONS -D_POSIX_THREADS : we want everything to be
#		 pthread safe.
DEFS=-DSTATIC= 

#
# TBD: Adding -O3 to FLAGS, DBG and DEFS will cause some tests to
# stop working properly. Debug this.
#

#
# IMPORTANT: Any flags/defines you want to be used for udibuild't
# objects MUST have those same flags/defines specified for the
# compile options in the udiprops.txt file!
#
# TBD: Make a way to specify the udibuild/udisetup flags here instead
# of in the udiprops.txt file.
#


TORTURE_RUN=../linux-user/torture_run

LIBS=$(LIBELFLIB) $(MALLOCDEBUGLIB)

