CC:sh=sh ../solaris/getarchflags -targetcc
#FLAGS:sh=sh ../solaris/getarchflags -targetccopts
FLAGS:sh=sh ../solaris/getarchflags -targetmapperopts
MACH:sh=mach
DBG=-g 
DEFS=-DSTATIC=static
RUN='../solaris-user/test_run'
TORTURE_RUN='../solaris-user/torture_run'
LINT=lint
OS_LINTFLAGS=-Dlint -x -y -u -v -m
#LIBS=-lpthread -lelf -L../solaris-user -lnull_sprops
LIBS=-lrt -lpthread -lelf -L../solaris-user -lnull_sprops -losdep_pio_$(MACH)
LINK_FLAGS=-g -Wl,-B,local

udi_sprops.o posix.o: ../solaris-user/libnull_sprops.a ../solaris-user/libosdep_pio_$(MACH).a

../solaris-user/libosdep_pio_$(MACH).a: ../solaris-user/libosdep_pio_$(MACH).a(osdep_pio_iout_$(MACH).o)
../solaris-user/libosdep_pio_$(MACH).a(osdep_pio_iout_$(MACH).o): ../solaris-user/osdep_pio_iout_$(MACH).s

../solaris-user/libnull_sprops.a: ../solaris-user/libnull_sprops.a(pseudod_null_props.o)
../solaris-user/libnull_sprops.a(pseudod_null_props.o): pseudod_null_props.c
pseudod_null_props.c: ../solaris-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_pseudod_sprops_idx[] = { {0, NULL} };" >> $@

../solaris-user/libnull_sprops.a: ../solaris-user/libnull_sprops.a(udi_nic_null_props.o)
../solaris-user/libnull_sprops.a(udi_nic_null_props.o): udi_nic_null_props.c
udi_nic_null_props.c: ../solaris-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_udi_nic_sprops_idx[] = { {0, NULL} };" >> $@

../solaris-user/libnull_sprops.a: ../solaris-user/libnull_sprops.a(netmap_null_props.o)
../solaris-user/libnull_sprops.a(netmap_null_props.o): netmap_null_props.c
netmap_null_props.c: ../solaris-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_netmap_sprops_idx[] = { {0, NULL} };" >> $@

../solaris-user/libnull_sprops.a: ../solaris-user/libnull_sprops.a(pseudond_null_props.o)
../solaris-user/libnull_sprops.a(pseudond_null_props.o): pseudond_null_props.c
pseudond_null_props.c: ../solaris-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_pseudond_sprops_idx[] = { {0, NULL} };" >> $@

../solaris-user/libnull_sprops.a: ../solaris-user/libnull_sprops.a(bridgemap_null_props.o)
../solaris-user/libnull_sprops.a(bridgemap_null_props.o): bridgemap_null_props.c
bridgemap_null_props.c: ../solaris-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_bridgemap_sprops_idx[] = { {0, NULL} };" >> $@

../solaris-user/libnull_sprops.a: ../solaris-user/libnull_sprops.a(bridge_driver_null_props.o)
../solaris-user/libnull_sprops.a(bridge_driver_null_props.o): bridge_driver_null_props.c
bridge_driver_null_props.c: ../solaris-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_bridge_driver_sprops_idx[] = { {0, NULL} };" >> $@

../solaris-user/libnull_sprops.a: ../solaris-user/libnull_sprops.a(udi_gio_null_props.o)
../solaris-user/libnull_sprops.a(udi_gio_null_props.o): udi_gio_null_props.c
udi_gio_null_props.c: ../solaris-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_giomap_sprops_idx[] = { {0, NULL} };" >> $@

../solaris-user/libnull_sprops.a: ../solaris-user/libnull_sprops.a(shrkudi_null_props.o)
../solaris-user/libnull_sprops.a(shrkudi_null_props.o): shrkudi_null_props.c
shrkudi_null_props.c: ../solaris-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_shrkudi_sprops_idx[] = { {0, NULL} };" >> $@

../solaris-user/libnull_sprops.a: ../solaris-user/libnull_sprops.a(udi_guinead_null_props.o)
../solaris-user/libnull_sprops.a(udi_guinead_null_props.o): udi_guinead_null_props.c
udi_guinead_null_props.c: ../solaris-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_udi_guinead_sprops_idx[] = { {0, NULL} };" >> $@

../solaris-user/libnull_sprops.a: ../solaris-user/libnull_sprops.a(cmosudi_null_props.o)
../solaris-user/libnull_sprops.a(cmosudi_null_props.o): cmosudi_null_props.c
cmosudi_null_props.c: ../solaris-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_udi_cmos_sprops_idx[] = { {0, NULL} };" >> $@


clobber: clean
	-rm -rf .*POSIX
	-rm -rf .udi_user_env
	-find . -type l | xargs rm -f
	-rm -f *_null_props.c
	-rm -rf msg
