#include <udi_osdep_opaquedefs.h>
#include <envcommon/udi_std_sprops.c>

_udi_std_mesgfile_t *
_OSDEP_GET_STD_MSGFILE(_udi_driver_t *drv, char *msgfile_name,
	udi_ubit32_t msgnum)
{
	_OSDEP_PRINTF("msgfile not implemented\n");
	return NULL;
}

void _udi_osdep_sprops_init(_udi_driver_t *drv)
{
}

void
_udi_osdep_sprops_deinit(_udi_driver_t *drv)
{
	udi_std_sprops_free(drv->drv_osinfo.sprops);
	drv->drv_osinfo.sprops = NULL;
}

