# Makefile for the UDI Prototype Development Effort: Metalanguage Libraries
#

SHELL=/bin/sh
TOPDIR=../..
OBJDIR=.build
INDIR=mapper/freebsd
SUBDIRS=bridge gio net # scsi

include $(TOPDIR)/env/common/Make_conf.common


DIRS=bridge gio net # scsi

configure:
	@for dir in $(DIRS); do \
	    if [ -d $$dir ] ; then \
		(cd $$dir && make); \
	    fi; \
	done

real-build real-all real-install::
	@:;    # do nothing
