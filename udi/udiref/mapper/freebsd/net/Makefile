#
# File: mapper/freebsd/net/Makefile
#
# Makefile for the FreeBSD NIC Mapper
#

#
# $Copyright udi_reference:
# $
#

SHELL=/bin/sh

build: netcommon envcommon env

netcommon:
	ln -s ../../common/net ./netcommon

envcommon:
	ln -s ../../../env/common ./envcommon

env:
	ln -s ../../../env/freebsd ./env

install:

clean:
	rm -rf bin *.udi netcommon envcommon env

