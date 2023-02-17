FLAGS=-Kthread -Kframe -v
DBG=-g -DDEBUG
DEFS=-DSTATIC=static
RUN='../uw-user/test_run'
TORTURE_RUN='../uw-user/torture_run'
#LINT=/g64/usr/ccs/bin/g64lint
LINT=lint
OS_LINTFLAGS=-Dlint -x -y -u -v -m
#LIBS=-L../uwstub -lnull_sprops
LIBS=-lelf -L../uw-user -lnull_sprops

udi_sprops.o posix.o: ../uw-user/libnull_sprops.a

../uw-user/libnull_sprops.a: ../uw-user/libnull_sprops.a(pseudod_null_props.o)
../uw-user/libnull_sprops.a(pseudod_null_props.o): pseudod_null_props.c
pseudod_null_props.c: ../uw-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_pseudod_sprops_idx[] = { {0, NULL} };" >> $@

../uw-user/libnull_sprops.a: ../uw-user/libnull_sprops.a(udi_nic_null_props.o)
../uw-user/libnull_sprops.a(udi_nic_null_props.o): udi_nic_null_props.c
udi_nic_null_props.c: ../uw-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_udi_nic_sprops_idx[] = { {0, NULL} };" >> $@

../uw-user/libnull_sprops.a: ../uw-user/libnull_sprops.a(netmap_null_props.o)
../uw-user/libnull_sprops.a(netmap_null_props.o): netmap_null_props.c
netmap_null_props.c: ../uw-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_netmap_sprops_idx[] = { {0, NULL} };" >> $@

../uw-user/libnull_sprops.a: ../uw-user/libnull_sprops.a(pseudond_null_props.o)
../uw-user/libnull_sprops.a(pseudond_null_props.o): pseudond_null_props.c
pseudond_null_props.c: ../uw-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_pseudond_sprops_idx[] = { {0, NULL} };" >> $@

../uw-user/libnull_sprops.a: ../uw-user/libnull_sprops.a(bridgemap_null_props.o)
../uw-user/libnull_sprops.a(bridgemap_null_props.o): bridgemap_null_props.c
bridgemap_null_props.c: ../uw-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_bridgemap_sprops_idx[] = { {0, NULL} };" >> $@

../uw-user/libnull_sprops.a: ../uw-user/libnull_sprops.a(bridge_driver_null_props.o)
../uw-user/libnull_sprops.a(bridge_driver_null_props.o): bridge_driver_null_props.c
bridge_driver_null_props.c: ../uw-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_bridge_driver_sprops_idx[] = { {0, NULL} };" >> $@

../uw-user/libnull_sprops.a: ../uw-user/libnull_sprops.a(udi_gio_null_props.o)
../uw-user/libnull_sprops.a(udi_gio_null_props.o): udi_gio_null_props.c
udi_gio_null_props.c: ../uw-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_giomap_sprops_idx[] = { {0, NULL} };" >> $@

../uw-user/libnull_sprops.a: ../uw-user/libnull_sprops.a(shrkudi_null_props.o)
../uw-user/libnull_sprops.a(shrkudi_null_props.o): shrkudi_null_props.c
shrkudi_null_props.c: ../uw-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_shrkudi_sprops_idx[] = { {0, NULL} };" >> $@

../uw-user/libnull_sprops.a: ../uw-user/libnull_sprops.a(udi_guinead_null_props.o)
../uw-user/libnull_sprops.a(udi_guinead_null_props.o): udi_guinead_null_props.c
udi_guinead_null_props.c: ../uw-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_udi_guinead_sprops_idx[] = { {0, NULL} };" >> $@

../uw-user/libnull_sprops.a: ../uw-user/libnull_sprops.a(cmosudi_null_props.o)
../uw-user/libnull_sprops.a(cmosudi_null_props.o): cmosudi_null_props.c
cmosudi_null_props.c: ../uw-user/udi_osdep.mk
	echo "#include <udi_env.h>" > $@
	echo "_udi_sprops_idx_t udi_udi_cmos_sprops_idx[] = { {0, NULL} };" >> $@

