#
# File: env/posix/Makefile
#
# Posix Makefile
# Used to build Posix test utilities for the UDI Prototype Code.
# Some make commands that are useful are:
#    build   : The default rule. Build all the source.
#    all     : Same as build.
#    clean   : Remove built components from your source tree.
#    distclean: Remove any ./configure'd components.
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

TOPDIR=../..
INDIR=env/posix
OBJDIR=.build
SHELL=/bin/sh

#
# First level default target
#

all: build

#
# For people who forget to run configure.
#

osenvdir/udi_osdep.mk:
	@echo "Please run ./configure"

#
# Standard definitions and locations
#

include osenvdir/udi_osdep.mk

INCDIRS=-I. -I../../pub_include -Iosenvdir -I../common -I../posix \
	-I../../mapper/common/gio -I../../mapper/posix/gio \
	-I../../mapper/common/net -I../../mapper/posix/net \
	-I../../driver/pseudond

CFLAGS=$(DBG) $(INCDIRS) $(DEFS) $(FLAGS) 

LINTFLAGS=$(CFLAGS) $(OS_LINTFLAGS)

PSEUDOND_CTRL_SCRATCH_SZ=${__PSEUDOND_CTRL_SCRATCH_SZ:-4}

PMETAD=$(TOPDIR)/meta
DRIVERD=$(TOPDIR)/driver

UDIGET=osenvdir/udiget


VPATH=.:osenvdir:\
       :../../mapper/posix/net:\
       :../../mapper/common/net:\
       :$(PMETAD)/mgmt:


META=	gio.o

#NETWORK=$(PMETAD)/nic/$(ABIBIN)/udi_nic netmap_posix.o

ENVIRONMENT=udi.o 

UDI=$(ENVIRONMENT) $(META) $(OSFILES) giomap.o

BRIDGE=bridgemap.o bridge.o
BRIDGE_DRV=bridgetest_driver.o

NETWORK=netmap.o nic.o

PD_UDI=$(UDI) $(BRIDGE) pseudod.o

ND_UDI=$(UDI) $(BRIDGE) $(NETWORK) pseudond.o

OSFILES=posix.o udi_MA_osdep.o posix_msgfile.o posix_sprops_elf.o

#
# Main list of things to be built
#

PROGS_NOT= $(HOST_TESTS) sptest # pcpiotest
PROGS=inittest unbindtest buffertest giotest constrainttest \
	cmostest ndtest piotest transtest bridgetest \
	inst_attrtest timertest guineatest

build: $(OSFILES) $(PROGS)

#
# Primary Targets
#

inittest: inittest.o $(PD_UDI)
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(PD_UDI) inittest.o $(LIBS) -o $@ 

unbindtest: unbindtest.o $(PD_UDI)
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(PD_UDI) unbindtest.o $(LIBS) -o $@ 

buffertest: buffertest.o $(UDI)
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) buffertest.o $(LIBS) -o $@ 

constrainttest: constrainttest.o $(UDI)
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) constrainttest.o $(LIBS) -o $@ 

cmostest: cmostest.o $(UDI) $(BRIDGE) udi_cmos.o
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) $(BRIDGE) cmostest.o udi_cmos.o $(LIBS) -o $@ 

pcitest: pcitest.o $(UDI) $(BRIDGE) readpci.o
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) $(BRIDGE) pcitest.o readpci.o $(LIBS) -o $@ 

shrktest: shrktest.o $(UDI) $(BRIDGE) $(NETWORK) shrkudi.o
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) $(BRIDGE) $(NETWORK) -I$(DRIVERD)/shrkudi shrkudi.o shrktest.o $(LIBS) -o $@ 

giotest: giotest.o $(PD_UDI)
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(PD_UDI) giotest.o $(LIBS) -o $@ 

piotest: piotest.o $(UDI)
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) piotest.o $(LIBS) -o $@ 

pcpiotest: pcpiotest.o $(UDI)
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) pcpiotest.o $(LIBS) -o $@ 

timertest: timertest.o $(UDI)
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) timertest.o $(LIBS) -o $@ 

transbench: transbench.o $(UDI) 
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) transbench.o $(LIBS) -o $@ 

transtest: transtest.o $(UDI)
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) transtest.o $(LIBS) -o $@ 

bridgetest: bridgetest.o $(UDI) $(BRIDGE) $(BRIDGE_DRV)
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) $(BRIDGE) $(BRIDGE_DRV) bridgetest.o $(LIBS) -o $@

guineatest: guineatest.o $(UDI) $(BRIDGE) udi_guinea.o
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) $(BRIDGE) udi_guinea.o guineatest.o $(LIBS) -o $@

ndtest: ndtest.o $(ND_UDI)
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(ND_UDI) ndtest.o $(LIBS) -o $@ 

inst_attrtest: inst_attrtest.o $(UDI)
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) inst_attrtest.o $(LIBS) -o $@

sptest: sptest.o $(UDI) sp_driver.o
	$(CC) $(LINK_FLAGS) $(CFLAGS) $(UDI) sptest.o sp_driver.o $(LIBS) -o $@

clean::
	rm -f *.o $(PROGS) $(META) $(PD_UDI) $(BRIDGE) mkpop osdep_pops.h
	find .* -name '*.o' -exec rm -f {} \;

# Remove the things that configure created.
distclean: clean
	rm -f real_osdep.h sprops.h udi_osdep.c udi_osdep.mk udi_sprops.c
	rm -f envcommon mappers osenvdir uw-user
	rm -rf .*_POSIX .udi_user_env

check: $(PROGS) check_nodep

check_nodep:
	@for i in $(PROGS) ; do \
	echo "*** Running: $$i" ; \
	$(RUN) ./$$i ; \
	if [ $$? -eq 0 ] ; then true; else exit 1; fi; \
	done
	@echo "***** All tests finished successfully *****"

quick_check: $(PROGS) quick_check_nodep

quick_check_nodep:
	@for i in $(PROGS) ; do \
	echo "*** Running: $$i" ; \
	./$$i ; \
	if [ $$? -eq 0 ] ; then true; else exit 1; fi; \
	done
	@echo "***** All tests finished successfully *****"

torture: $(PROGS) torture_nodep

torture_nodep:
	@for i in $(PROGS) ; do \
	$(TORTURE_RUN) ./$$i ; \
	if [ $$? -eq 0 ] ; then true; else exit 1; fi; \
	done
	@echo "***** All tests finished successfully *****"


#
# Support Targets
#

$(UDIGET): ../../tools/bin/udibuild ../../tools/bin/udimkpkg

../../tools/bin/udibuild ../../tools/bin/udimkpkg: FRC
	@(cd ../../tools && $(MAKE))

#
# If you want to enable debugging or optimization flags for
# udibuild, change the following variables.
#
UDIBUILD_FLAGS_MAPPER=-m
UDIBUILD_FLAGS=

#
# Similarly, the following flags can be used for the udimkpkg stage
#
UDIMKPKG_FLAGS_MAPPER=-m
UDIMKPKG_FLAGS=

#udi.o: osenvdir/udi.udi
udi.o: $(UDIGET) FRC
	@(cd osenvdir && make)
	@$(UDIGET) $@ udi_user_env osenvdir `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS_MAPPER) $(UDIMKPKG_FLAGS_MAPPER)

#giomap.o: mappers mappers/gio/giomap.udi
giomap.o: $(UDIGET) FRC
	@(cd mappers && make configure)
	@$(UDIGET) $@ giomap_POSIX mappers/gio  `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS_MAPPER) $(UDIMKPKG_FLAGS_MAPPER)

#bridgemap.o: mappers mappers/bridge/bridgemap.udi
bridgemap.o: $(UDIGET) FRC
	@(cd mappers && make configure)
	@$(UDIGET) $@ brdg_map_POSIX mappers/bridge  `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS_MAPPER) $(UDIMKPKG_FLAGS_MAPPER)

#netmap.o: mappers mappers/net/netmap.udi
netmap.o: FRC
	@(cd mappers && make configure)
	@$(UDIGET) $@ netmap_POSIX mappers/net `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS_MAPPER) $(UDIMKPKG_FLAGS_MAPPER)

#udi_guinea.o: $(DRIVERD)/udi_guinea/udi_guinea.c \
#			 $(DRIVERD)/udi_guinea/udi_guinea.h \
#			 $(DRIVERD)/udi_guinea/udiprops.txt
udi_guinea.o: FRC
	@$(UDIGET) $@ udi_gpig_POSIX $(DRIVERD)/udi_guinea `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS) $(UDIMKPKG_FLAGS)

#sp_driver.o: $(DRIVERD)/posix_sptest/sp_driver.c \
#			$(DRIVERD)/posix_sptest/sp_extra.c \
#			$(DRIVERD)/posix_sptest/udiprops.txt \
#			$(DRIVERD)/posix_sptest/read_file \
#			$(DRIVERD)/posix_sptest/gibberish
sp_driver.o: FRC
	@$(UDIGET) $@ sp_driver_POSIX $(DRIVERD)/posix_sptest `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS) $(UDIMKPKG_FLAGS)

#bridgetest_driver.o: $(DRIVERD)/posix_bridge/bridgetest_driver.c \
#				$(DRIVERD)/posix_bridge/udiprops.txt
bridgetest_driver.o: FRC
	@$(UDIGET) $@ brdg_drv_POSIX $(DRIVERD)/posix_bridge `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS) $(UDIMKPKG_FLAGS)

#pseudod.o: $(DRIVERD)/pseudo/pseudod.udi
pseudod.o: FRC
	@$(UDIGET) $@ pseudod_POSIX $(DRIVERD)/pseudo `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS) $(UDIMKPKG_FLAGS)

#shrkudi.o: $(DRIVERD)/shrkudi/shrkudi.udi
shrkudi.o: FRC
	@$(UDIGET) $@ shrkudi_POSIX $(DRIVERD)/shrkudi `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS) $(UDIMKPKG_FLAGS)

#pseudond.o: $(DRIVERD)/pseudond/pseudond.udi
pseudond.o: FRC
	@$(UDIGET) $@ pseudond_POSIX $(DRIVERD)/pseudond `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS) $(UDIMKPKG_FLAGS)

#bridge.o: $(PMETAD)/bridge/udi_bridge.udi
bridge.o: FRC
	@$(UDIGET) $@ udi_brdg_POSIX $(PMETAD)/bridge `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS) $(UDIMKPKG_FLAGS)

#nic.o: $(PMETAD)/network/udi_nic.udi
nic.o: FRC
	@$(UDIGET) $@ udi_nic_POSIX $(PMETAD)/network `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS) $(UDIMKPKG_FLAGS)

#gio.o: $(PMETAD)/gio/udi_gio.udi
gio.o: FRC
	@$(UDIGET) $@ udi_gio_POSIX $(PMETAD)/gio `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS) $(UDIMKPKG_FLAGS)

#udi_cmos.o: $(DRIVERD)/udi_cmos/udi_cmos.udi
udi_cmos.o: FRC
	@$(UDIGET) $@ udi_cmos_POSIX $(DRIVERD)/udi_cmos `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS) $(UDIMKPKG_FLAGS)

readpci.o: FRC
	$(UDIGET) $@ readpci_POSIX $(DRIVERD)/examples/readpci `pwd`/$(TOPDIR)/ $(UDIBUILD_FLAGS) $(UDIMKPKG_FLAGS)

# This target depends on having GCC availble.  Fortunately, the 
# output (after edits) is portable to all makes and compilers.
# The sed command replaces all absolute pathnames with spaces
# (they're likely to be GCC internals or system headers)
# and then deletes lines consisting of only blank continuations.
depend:
	rm -f *.d > /dev/null 2>&1
	-type gcc > /dev/null 2>&1; rc=$$?; \
	echo "rc=$$rc"; \
	if [ $$rc -eq 0 ]; then \
		echo "Creating new depend.mk file."; \
		$(MAKE) clean all FLAGS="-fsyntax-only -MD" CC=gcc; \
		( echo '# This file is machine generated.  Do not edit.' ; \
		cat *.d  | sed 's@ /.* @ @;s@ /.*$$@ @;s@ \\\\n\\n@\\n@;/^ \\$$/d;' | \
		awk 'BEGIN {getline; inp=$$0;} \
			{ \
				if ($$0 != " ") { \
					if (inp != " ") {print inp;} \
				} else { \
					print substr(inp, 1, length(inp)-1); \
				} \
				inp=$$0; \
			} \
		     END {print inp}' \
		)> depend.mk; \
	else \
		echo "NOTE: gcc unavailable.  Using default depend.mk file."; \
	fi
	$(MAKE) clean ;  rm -f *.d


include depend.mk

FRC:


#
# Explicit dependencies not part of this directory
#


