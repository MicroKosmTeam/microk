#
# File: tools/Makefile
#      
# Makefile for the UDI Prototype Development Effort: UDI Tools
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



SHELL=/bin/sh
TOPDIR=..
ENVDIR=`pwd`/..
OBJDIR=.build

OS_RULES=common/tools.mk
include $(OS_RULES)

INDIR=tools
BINDIR=$(TOPDIR)/$(INDIR)/bin

# This is a makefile which is invoked from the primary makefile after that
# makefile sets the appropriate environment variables (via including the
# env/common/Make_conf.common file) before invoking this make with the
# build target:
#    INDIR  = (TOPDIR relative) directory to do actual make in when configured
#    TOPDIR = relation of INDIR to the top of the source tree
#    OBJDIR = (INDIR relative) directory to build objects in
#    OS_ENV = OS/architecture unique identifier
#    OS_NAME = OS short name (matches directory qualifiers)
#    CC = name of compiler
#    CFLAGS = flags to pass to compiler
#    CINCL = include directives for compiler
#    PUB_DIR = pub_include directory (absolute path name)
#    PUB_HDRS = list of env services & Mgmt Meta pub_include files

LOCALCFLAGS=-Icommon -I$(OS_NAME) -I$(TOPDIR)/abi/$(UDI_ABI) -I$(TOPDIR)/pub_include

# Clear current make control variables so that sub-invoked make operations
# get to re-compute their flags accordingly.  This is needed for makes that
# operate similar to the -e: AIX make, GNU make, etc.
MAKEFLAGS=
MAKEOVERRIDES=


MKPKGOBJS= \
	$(OS_COMMON_OBJS) \
	$(OS_NAME)/y.tab.c \
	$(OS_NAME)/lex.yy.c \
	$(OBJDIR)/set_abi-$(OS_ENV).o \
	$(OBJDIR)/common-$(OS_ENV).o \
	$(OBJDIR)/mkpkgparam-$(OS_ENV).o \
	$(OBJDIR)/std_exec-$(OS_ENV).o \
	$(OBJDIR)/udimkpkg-$(OS_ENV).o


MKBLDOBJS= \
	$(OS_COMMON_OBJS) \
	$(OS_NAME)/y.tab.c \
	$(OS_NAME)/lex.yy.c \
	$(OBJDIR)/set_abi-$(OS_ENV).o \
	$(OBJDIR)/common-$(OS_ENV).o \
	$(OBJDIR)/buildparam-$(OS_ENV).o \
	$(OBJDIR)/std_exec-$(OS_ENV).o \
	$(OBJDIR)/compile-$(OS_ENV).o \
	$(OBJDIR)/udibuild-$(OS_ENV).o


MKSETOBJS= \
	$(OS_COMMON_OBJS) \
	$(OS_NAME)/y.tab.c \
	$(OS_NAME)/lex.yy.c \
	$(OBJDIR)/common-$(OS_ENV).o \
	$(OBJDIR)/setupparam-$(OS_ENV).o \
	$(OBJDIR)/std_exec-$(OS_ENV).o \
	$(OBJDIR)/set_abi-$(OS_ENV).o \
	$(OBJDIR)/setup_glue-$(OS_ENV).o \
	$(OBJDIR)/osdep_make-$(OS_ENV).o \
	$(OBJDIR)/udisetup-$(OS_ENV).o

EXECS= \
	$(BINDIR)/udimkpkg-$(OS_ENV) \
	$(BINDIR)/udibuild-$(OS_ENV) \
	$(BINDIR)/udisetup-$(OS_ENV) 

real-build:: $(BINDIR) $(OS_NAME)/osdep.h \
	$(BINDIR)/udicmds $(BINDIR)/udicmds.msg \
	$(MKPKGOBJS) $(MKBLDOBJS) $(MKSETOBJS) $(EXECS) 


MSG_FILES=$(OBJDIR)/common.msg  $(OBJDIR)/spparsey.msg \
	  $(OBJDIR)/osdep_make.msg $(OBJDIR)/std_exec.msg \
	  $(OBJDIR)/param.msg $(OBJDIR)/udibuild.msg \
	  $(OBJDIR)/set_abi.msg $(OBJDIR)/udimkpkg.msg \
	  $(OBJDIR)/setup_glue.msg $(OBJDIR)/udisetup.msg \
	  $(OBJDIR)/spparsel.msg $(OBJDIR)/compile.msg

$(BINDIR)/udicmds: common/udicmds $(BINDIR)/udicmds.msg
	cp common/udicmds $(BINDIR)/udicmds

common/udicmds: $(BINDIR)/udicmds.msg
	@ if [ "X$(GEN_MSG_CAT)" != "X" ]; then \
		rm -f common/udicmds ; \
		echo "        $(GEN_MSG_CAT) common/udicmds $(BINDIR)/udicmds.msg" ; \
		$(GEN_MSG_CAT) common/udicmds $(BINDIR)/udicmds.msg && \
		( echo "        cp common/udicmds $(BINDIR)/udicmds" ; \
		cp common/udicmds $(BINDIR)/udicmds ) ; \
	fi

$(BINDIR)/udicmds.msg: $(MSG_FILES)
	perl common/getmsgs.pl chkmsg copyrights/msgs/udi_reference \
		$(MSG_FILES) > $(BINDIR)/udicmds.msg;

distclean: clean
	rm -rf $(BINDIR)/*

$(BINDIR):
	mkdir $(BINDIR)

$(BINDIR)/udimkpkg: 
	echo "#!/bin/sh" > $(BINDIR)/udimkpkg
	echo 'LOC=`dirname $$0`' >> $(BINDIR)/udimkpkg
	echo 'sh -c "$$LOC/udimkpkg-`sh $$LOC/../../env/common/Make_conf -E` $$*"' >> $(BINDIR)/udimkpkg
	chmod 775 $(BINDIR)/udimkpkg

$(BINDIR)/udibuild:
	sed -e 's/udimkpkg/udibuild/g' $(BINDIR)/udimkpkg > $(BINDIR)/udibuild
	chmod 775 $(BINDIR)/udibuild

$(BINDIR)/udisetup:
	sed -e 's/udimkpkg/udisetup/g' $(BINDIR)/udimkpkg > $(BINDIR)/udisetup
	chmod 775 $(BINDIR)/udisetup

$(BINDIR)/udimkpkg-$(OS_ENV): $(MKPKGOBJS) $(BINDIR)/udimkpkg \
		$(OS_UDIBUILD_OBJS)
	$(CC) -o $@ $(CFLAGS) -I$(TOPDIR)/env/common \
		$(MKPKGOBJS) $(OS_UDIBUILD_OBJS) $(LIB_LEX) $(LIB_OBJ)
	chmod 755 $@

$(BINDIR)/udibuild-$(OS_ENV): $(MKBLDOBJS) $(BINDIR)/udibuild \
		 $(OS_UDIBUILD_OBJS)
	$(CC) -o $@ $(CFLAGS) -I$(TOPDIR)/env/common \
		$(MKBLDOBJS) $(OS_UDIBUILD_OBJS) $(LIB_LEX) $(LIB_OBJ)
	chmod 755 $@

$(BINDIR)/udisetup-$(OS_ENV): $(MKSETOBJS) $(OS_NAME)/osdep.h \
				  $(BINDIR)/udisetup $(OS_UDIBUILD_OBJS)
	$(CC) -o $@ $(CFLAGS) -I$(TOPDIR)/env/common \
		$(MKSETOBJS) $(OS_UDIBUILD_OBJS) $(LIB_LEX) $(LIB_OBJ)
	chmod 755 $@

PARSE_H=$(OS_NAME)/y.tab.h common/spparse.h $(OS_NAME)/osdep.h \
	common/global.h common/param.h

BUILDOP=buildcc() {  echo "        $(CC) $(CFLAGS) $$5 -c -o $$1 $$2/$$3" ; \
	$(CC) $(CFLAGS) $$5 -c -o $$1 $$2/$$3 && \
	perl common/getmsgs.pl getmsg \
		copyrights/msgs/udi_reference  $$2/$$3 > $$2/$$4 && \
	cp $$2/$$4 $(OBJDIR)/$$4 ; \
	}

$(OBJDIR)/udimkpkg.msg: $(OBJDIR)/udimkpkg-$(OS_ENV).o
$(OBJDIR)/udimkpkg-$(OS_ENV).o: common/udimkpkg.c $(PARSE_H) \
				../abi/$(UDI_ABI)/udi_abi.h
	@ $(BUILDOP) ; buildcc $@ common udimkpkg.c udimkpkg.msg

$(OBJDIR)/udibuild.msg: $(OBJDIR)/udibuild-$(OS_ENV).o
$(OBJDIR)/udibuild-$(OS_ENV).o: common/udibuild.c $(PARSE_H) \
				../abi/$(UDI_ABI)/udi_abi.h
	@ $(BUILDOP) ; buildcc $@ common udibuild.c udibuild.msg
 
$(OBJDIR)/udisetup.msg: $(OBJDIR)/udisetup-$(OS_ENV).o
$(OBJDIR)/udisetup-$(OS_ENV).o: common/udisetup.c $(PARSE_H) $(OS_NAME)/osdep.h
	@ $(BUILDOP) ; buildcc $@ common udisetup.c udisetup.msg

$(OBJDIR)/common.msg: $(OBJDIR)/common-$(OS_ENV).o
$(OBJDIR)/common-$(OS_ENV).o: common/common.c common/spparse.h \
			      $(OS_NAME)/osdep.h common/param.h \
			      common/global.h \
			      $(TOPDIR)/abi/$(UDI_ABI)/udi_abi.h \
			      $(OS_NAME)/y.tab.h
	@ $(BUILDOP) ; buildcc $@ common common.c common.msg

$(OBJDIR)/compile.msg: $(OBJDIR)/compile-$(OS_ENV).o
$(OBJDIR)/compile-$(OS_ENV).o: common/compile.c \
			      $(OS_NAME)/osdep.h \
			      common/global.h \
			      $(TOPDIR)/abi/$(UDI_ABI)/udi_abi.h \
			      $(OS_NAME)/y.tab.h
	@ $(BUILDOP) ; buildcc $@ common compile.c compile.msg

$(OBJDIR)/param.msg: $(OBJDIR)/mkpkgparam-$(OS_ENV).o
$(OBJDIR)/mkpkgparam-$(OS_ENV).o: common/param.c common/param.h \
				  $(OS_NAME)/osdep.h common/global.h
	@ $(BUILDOP) ; buildcc $@ common param.c param.msg -D_UDIMKPKG

$(OBJDIR)/param.msg: $(OBJDIR)/buildparam-$(OS_ENV).o
$(OBJDIR)/buildparam-$(OS_ENV).o: common/param.c common/param.h \
				  $(OS_NAME)/osdep.h common/global.h
	@ $(BUILDOP) ; buildcc $@ common param.c param.msg -D_UDIBUILD

$(OBJDIR)/param.msg: $(OBJDIR)/setupparam-$(OS_ENV).o
$(OBJDIR)/setupparam-$(OS_ENV).o: common/param.c common/param.h \
				  $(OS_NAME)/osdep.h common/global.h
	@ $(BUILDOP) ; buildcc $@ common param.c param.msg -D_UDISETUP

$(OBJDIR)/std_exec.msg: $(OBJDIR)/std_exec-$(OS_ENV).o
$(OBJDIR)/std_exec-$(OS_ENV).o: common/std_exec.c common/global.h \
				  $(OS_NAME)/osdep.h 
	@ $(BUILDOP) ; buildcc $@ common std_exec.c std_exec.msg

$(OBJDIR)/set_abi.msg: $(OBJDIR)/set_abi-$(OS_ENV).o
$(OBJDIR)/set_abi-$(OS_ENV).o: common/set_abi.c common/global.h \
				  $(OS_NAME)/osdep.h 
	@ $(BUILDOP) ; buildcc $@ common set_abi.c set_abi.msg

$(OBJDIR)/setup_glue.msg: $(OBJDIR)/setup_glue-$(OS_ENV).o
$(OBJDIR)/setup_glue-$(OS_ENV).o: common/setup_glue.c common/global.h \
				  $(OS_NAME)/osdep.h 
	@ $(BUILDOP) ; buildcc $@ common setup_glue.c setup_glue.msg

$(OBJDIR)/osdep_make.msg: $(OBJDIR)/osdep_make-$(OS_ENV).o
$(OBJDIR)/osdep_make-$(OS_ENV).o: $(OS_NAME)/osdep_make.c
	@ $(BUILDOP) ; buildcc $@ $(OS_NAME) osdep_make.c osdep_make.msg -I$(TOPDIR)/env/common

$(OS_NAME)/y.tab.c $(OS_NAME)/y.tab.h $(OBJDIR)/spparsey.msg: common/spparse.y common/spparse.h
	$(YACC) -d common/spparse.y
	mv y.tab.[ch] $(OS_NAME)/
	perl common/getmsgs.pl getmsg copyrights/msgs/udi_reference \
	     common/spparse.y > common/spparsey.msg ; \
	cp common/spparsey.msg $(OBJDIR)/

$(OS_NAME)/lex.yy.c $(OBJDIR)/spparsel.msg: common/spparse.l common/spparse.h $(OS_NAME)/y.tab.h  common/parser.c
	$(LEX) common/spparse.l
	mv lex.yy.c $(OS_NAME)/
	perl common/getmsgs.pl getmsg copyrights/msgs/udi_reference \
	     common/spparse.l > common/spparsel.msg ; \
	cp common/spparsel.msg $(OBJDIR)/

real-all:	real-build

real-install:
	[  -z "$$ROOT" ] && { \
		cp $(BINDIR)/udibuild-$(OS_ENV) /usr/bin/udibuild; \
		cp $(BINDIR)/udimkpkg-$(OS_ENV) /usr/bin/udimkpkg; \
		cp $(BINDIR)/udisetup-$(OS_ENV) /usr/bin/udisetup; \
	} ; exit 0
	[ ! -d /usr/include/udi ] && mkdir -p /usr/include/udi ; exit 0
	(cd $(TOPDIR)/pub_include; find . -print | grep -v CVS | cpio -d -p -u /usr/include/udi ;)


clean:
	-rm -f y.tab.o lex.yy.o $(GET_OS_NAME)/y.tab.c $(GET_OS_NAME)/y.tab.h */lex.yy.c
	-rm -f */*.o  $(BINDIR)/u*
	-find . -name .build | xargs rm -rf 2> /dev/null

FRC:
