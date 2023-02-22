#
# File: driver/Makefile
#
# Makefile for the UDI Prototype Development Effort: Portable Drivers
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

SHELL=/bin/sh
TOPDIR=..
ENVDIR=`pwd`/..
OBJDIR=.build
INDIR=driver
SUBDIRS=inph_fe pseudo pseudond udi_cmos udi_dpt shrkudi adptpci tstI_gio \
	udi_mega udi_guinea
SUBDIRS=eeeudi e1kudi


#
# A list of subdirs to be excluded from the default build.
#
EXCLUDE_DIRS=posix_bridge posix_sptest

#
# Build a list of directories containing 'udiprops.txt', collate it with the
# above list, then use only the directories that are NOT duplicated.
# (This is the sort of thing that makes me simultaneously proud and ashamed
# to be a UNIX guy...)
#
SUBDIRS=`(ls -1 */udiprops.txt | awk -F/ '{print $$1 }' ; for i in  $(EXCLUDE_DIRS); do echo $$i ; done ) | sort | uniq -u`

#
# Or, also, add to the directory list those directories that have Makefile's.
# The usage for this functionality is to have a Makefile containing:
# build:
#
# install:
# 	udisetup <driver>
# When a "make install" is done against the drivers directory in the UDI
# reference source tree, the <driver> will be installed. Now, you'll never
# forget to rerun udisetup against your prepackaged (source-only or
# binary-only) drivers.
#
#SUBDIRS=`(ls -1 */udiprops.txt */Makefile | awk -F/ '{print $$1 }' ; for i in  $(EXCLUDE_DIRS); do echo $$i ; done ) | sort | uniq -u`

#
# For rapid development purposes (perhaps you care about only a single driver)
#  you may list that driver[s] here and uncomment it.   But if you check that
#  in, I will track you down and beat you up.
#

#SUBDIRS=pseudo or whatever

include $(TOPDIR)/env/common/Make_conf.common

