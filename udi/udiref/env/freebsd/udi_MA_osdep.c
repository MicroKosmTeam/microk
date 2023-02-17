/*
 * File: env/freebsd/udi_MA_osdep.c
 *
 * FreeBSD Management Agent Handling
 */

/*
 * $Copyright udi_reference:
 * 
 * 
 *    Copyright (c) 1995-2001; Compaq Computer Corporation; Hewlett-Packard
 *    Company; Interphase Corporation; The Santa Cruz Operation, Inc;
 *    Software Technologies Group, Inc; and Sun Microsystems, Inc
 *    (collectively, the "Copyright Holders").  All rights reserved.
 * 
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the conditions are met:
 * 
 *            Redistributions of source code must retain the above
 *            copyright notice, this list of conditions and the following
 *            disclaimer.
 * 
 *            Redistributions in binary form must reproduce the above
 *            copyright notice, this list of conditions and the following
 *            disclaimers in the documentation and/or other materials
 *            provided with the distribution.
 * 
 *            Neither the name of Project UDI nor the names of its
 *            contributors may be used to endorse or promote products
 *            derived from this software without specific prior written
 *            permission.
 * 
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *    "AS IS," AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *    HOLDERS OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 *    DAMAGE.
 * 
 *    THIS SOFTWARE IS BASED ON SOURCE CODE PROVIDED AS A SAMPLE REFERENCE
 *    IMPLEMENTATION FOR VERSION 1.01 OF THE UDI CORE SPECIFICATION AND/OR
 *    RELATED UDI SPECIFICATIONS. USE OF THIS SOFTWARE DOES NOT IN AND OF
 *    ITSELF CONSTITUTE CONFORMANCE WITH THIS OR ANY OTHER VERSION OF ANY
 *    UDI SPECIFICATION.
 * 
 * 
 * $
 */


#include <udi_env.h>


static int
_udi_module_is_meta(char *sprops_text, int sprops_size, char **provides)
{
	char *sprops;
	int idx = 0;
	int i;

	sprops = sprops_text;
	for (idx = 0; idx < sprops_size; i++) {
		if (strncmp(&sprops[idx], "provides ",9) == 0) {
			*provides = &sprops[idx+9];
			return TRUE;
		}
		idx = idx + strlen(&sprops[idx]) + 1;
	}
	*provides = NULL;
	return FALSE;
}

void _udi_MA_osdep_init (void)
{
	_OSDEP_DRIVER_T drv_info;
	extern udi_mei_init_t udi_meta_info;
	extern char *_udi_sprops_scn_for_udi;
	extern char *_udi_sprops_scn_end_for_udi;
	int sprops_size;

	sprops_size = (int) &_udi_sprops_scn_end_for_udi - 
			  (int)&_udi_sprops_scn_for_udi;

	drv_info.sprops = udi_std_sprops_parse((char *)&_udi_sprops_scn_for_udi,
						sprops_size,
						"udi");
	_udi_MA_meta_load ("udi_mgmt", &udi_meta_info, drv_info);
}

int
_udi_driver_load(char *modname,
		 udi_init_t *init_info,
		 char *sprops_scn,
		 int sprops_size,
		 void * dev)
{
	_udi_driver_t *drv;
	udi_status_t r;
	_OSDEP_DRIVER_T default_osdep_driver;

	/* 
	 * dev is actually an device_t...It was handed to us in the attach
	 * routine.  Since it's key to all physical I/O in freebsd, we snag
	 * it now.
	 */
	
uprintf("Driver_load '%s' %p, %p, %x, %p\n", modname, init_info, sprops_scn, sprops_size, dev);
	default_osdep_driver.sprops = udi_std_sprops_parse( sprops_scn,
						sprops_size, modname);
	default_osdep_driver.device = dev;
	drv = _udi_MA_driver_load(modname, init_info, default_osdep_driver);
	r = _udi_MA_install_driver(drv);

	if (r != UDI_OK) {	/* Creation failed. */
uprintf("install_drv failed\n");
		_udi_MA_destroy_driver_by_name(modname);
	}

	return UDI_OK;
}
int
_udi_driver_unload(char *modname)
{
uprintf("Driver unload of %s\n", modname);
	_udi_MA_destroy_driver_by_name(modname);
	return 0;
}
		  
int
_udi_meta_load(char *modname,
		 udi_mei_init_t *meta_info,
		 char *sprops_scn,
		 int sprops_size)
{
	_OSDEP_DRIVER_T default_osdep_driver;
	char *provided_meta;
	udi_boolean_t is_meta;
	char provided_metab[512];
	char *s, *d;

	default_osdep_driver.sprops = udi_std_sprops_parse( sprops_scn,
						sprops_size, modname);
	is_meta = _udi_module_is_meta(default_osdep_driver.sprops->props_text,
				      default_osdep_driver.sprops->props_len, 
				      &provided_meta);

	/*
	 * Copy the 'provides' name from the sprops to a local
	 * buffer.   This is  temporary until we can add an
	 * 'sprops_get_provides...' facility.
	 */

	s = provided_meta;
	d = provided_metab;
	while (*s != ' ') {
		*d++ = *s++;
	}
	*d = '\0';

	_udi_MA_meta_load(provided_metab, meta_info, default_osdep_driver);

	return UDI_OK;
}

int
_udi_meta_unload(char *modname)
{
	 _udi_MA_meta_unload(modname);
	return 0;
}
		  
