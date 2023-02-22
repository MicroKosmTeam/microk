/*
 * File: env/freebsd/udi_osdep_inst_attr.c
 *
 * FreeBSD instance attribute handling.
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

#include <sys/bus.h>
#include <machine/bus.h>
#include <sys/rman.h>
#include <machine/resource.h>

void
_udi_dev_node_init(_udi_driver_t *driver,
                   _udi_dev_node_t *udi_node)
{                                       /* New udi node */
        _OSDEP_DEV_NODE_T *osinfo = &udi_node->node_osinfo;

	if (driver) {
		osinfo->device = driver->drv_osinfo.device;
	}
}

void
_udi_dev_node_deinit(_udi_dev_node_t *dev)
{
	int i;
	for (i = 0; i < sizeof(dev->node_osinfo.bus_resource) / 
		sizeof(dev->node_osinfo.bus_resource[0]); i++) {
		int rid = i * 4 + 0x10 ;
		if (dev->node_osinfo.bus_resource[i]) {
			bus_release_resource(dev->node_osinfo.device,
				SYS_RES_MEMORY, rid, 
				dev->node_osinfo.bus_resource[i]);
			
		}
	}
}

udi_size_t
_osdep_instance_attr_get(_udi_dev_node_t *node,
                         const char *name,
                         void *value,
                         udi_size_t length,
                         udi_ubit8_t *type)
{
	/* 
	 * FIXME: Implement real function.    But have to actively
	 * nack this for now.
	 */
	*type = UDI_ATTR_NONE;
	return 0;
}

udi_status_t
_osdep_instance_attr_set(_udi_dev_node_t *node,
                         const char *name,
                         const void *value,
                         udi_size_t length, udi_ubit8_t type)
{
	return UDI_OK;
}

