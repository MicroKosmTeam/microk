# Makefile for the UDI Prototype Development Effort: Metalanguage Libraries
#
# $Id: Makefile,v 1.1.1.1 2001/02/09 18:52:02 robertl Exp $

SHELL=/bin/sh
TOPDIR=../..
OBJDIR=.build
INDIR=mapper/solaris-user
SUBDIRS=bridge gio net #scsi

include $(TOPDIR)/env/common/Make_conf.common


DIRS=bridge gio net scsi

real-build real-all real-install::
	@:;    # do nothing

configure:
	@for dir in $(DIRS); do \
	    if [ -d $$dir ] ; then \
		(cd $$dir && make); \
	    fi; \
	done

clobber:
	@for dir in $(DIRS); do \
	    if [ -d $$dir ] ; then \
		(cd $$dir && make clean); \
	    fi; \
	done

