#
# Template (shared) Makefile for driver builds.
# There should be no reason to have to special case anything in this file
# for any driver other than the target stamp files which list the dependencies.
#

SHELL=/bin/sh
TOPDIR=../..
OBJDIR=.build
INDIR=driver

OS_RULES=$(TOPDIR)/env/common/Make_conf.common
include $(OS_RULES)

STAMPDIR=$(TOPDIR)/$(INDIR)/$(OBJDIR)
OUTDIR=$(STAMPDIR)/$(DEP)-$(OS_ENV)
STAMPFILE=$(STAMPDIR)/$(DEP)-$(OS_ENV)-stamp


real-build: $(STAMPFILE)
	rm -f real-build 
	-mkdir -p $(OUTDIR)
	$(UDIBUILD_BIN) -o $(OUTDIR)
	cp udiprops.txt $(OUTDIR)
	cd $(OUTDIR) ; $(UDIMKPKG_BIN) -t binary
	# mv udi-pkg.1 $(OUTDIR)
	touch real-build

real-install: real-build
	cd $(OUTDIR) ; $(UDISETUP_BIN)

clean::
	rm -fr udi-pkg.1 *.o real-build

$(STAMPDIR)/adptpci-$(OS_ENV)-stamp: adptpci.c \
	adptpci.h adptregs.h adptseq.h udiprops.txt
	touch $@
$(STAMPDIR)/pseudo-$(OS_ENV)-stamp: pseudo.c pseudo.h udiprops.txt
	touch $@
$(STAMPDIR)/pseudond-$(OS_ENV)-stamp: pseudond.c pseudond.h udiprops.txt
	touch $@
$(STAMPDIR)/shrkudi-$(OS_ENV)-stamp: shrkudi.c shrk.h udiprops.txt
	touch $@
$(STAMPDIR)/udi_cmos-$(OS_ENV)-stamp: udi_cmos.c udiprops.txt
	touch $@
$(STAMPDIR)/udi_dpt-$(OS_ENV)-stamp: udi_dpt.c udi_dpt.h udiprops.txt
	touch $@
