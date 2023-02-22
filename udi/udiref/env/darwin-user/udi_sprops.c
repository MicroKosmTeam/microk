#include <udi_env.h>

void
_udi_osdep_sprops_init(_udi_driver_t *drv)
{
}

void
_udi_osdep_sprops_deinit(_udi_driver_t *drv)
{
	udi_std_sprops_free(drv->drv_osinfo.sprops);
	drv->drv_osinfo.sprops = NULL;
}
