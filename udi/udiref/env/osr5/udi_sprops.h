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
/*
 * Written for SCO by: Rob Tarte
 * At: Pacific CodeWorks, Inc.
 * Contact: http://www.pacificcodeworks.com
 */

#ifndef _UDI_SPROPS_H
#define _UDI_SPROPS_H

/*
 * Function declarations
 */

char *
_udi_osdep_sprops_get_shortname (_udi_driver_t *drv);

void
_udi_osdep_sprops_set (_udi_driver_t *drv, void *sprops_idx);

udi_ubit32_t
_udi_osdep_sprops_get_version (_udi_driver_t *drv);

udi_ubit32_t
_udi_osdep_sprops_get_release (_udi_driver_t *drv);

udi_ubit32_t
_osdep_sprops_get_num (_udi_driver_t *drv, udi_ubit8_t type);

udi_ubit32_t
_udi_osdep_sprops_get_meta_idx (_udi_driver_t *drv, udi_ubit32_t index);

char *
_udi_osdep_sprops_get_meta_if (_udi_driver_t *drv, udi_ubit32_t index);

int
_udi_osdep_sprops_get_bindops (_udi_driver_t *drv, udi_ubit32_t index,
				udi_ubit8_t bind_type, udi_ubit8_t op_type);
char *
_udi_osdep_sprops_get_prov_if (_udi_driver_t *drv, udi_ubit32_t index);

udi_ubit32_t
_udi_osdep_sprops_get_prov_ver (_udi_driver_t *drv, udi_ubit32_t index);

char *
_udi_osdep_sprops_get_require_if (_udi_driver_t *drv, udi_ubit32_t index);

udi_ubit32_t
_udi_osdep_sprops_get_require_ver (_udi_driver_t *drv, udi_ubit32_t index);

udi_ubit32_t
_udi_osdep_sprops_get_reg_idx (_udi_driver_t *drv, udi_ubit32_t index);

udi_ubit32_t
_udi_osdep_sprops_get_reg_attr (_udi_driver_t *drv, udi_ubit32_t index);

udi_ubit32_t
_udi_osdep_sprops_get_reg_overrun (_udi_driver_t *drv, udi_ubit32_t index);

udi_ubit32_t
_udi_osdep_sprops_get_dev_meta_idx (_udi_driver_t *drv, udi_ubit32_t index);

udi_ubit32_t
_udi_osdep_sprops_get_dev_nattr (_udi_driver_t *drv, udi_ubit32_t index);

void *
_udi_osdep_sprops_get_dev_attr (_udi_driver_t *drv,
		udi_ubit32_t dev_index, udi_ubit32_t att_index);

void
_udi_osdep_sprops_get_dev_attr_name (_udi_driver_t *drv,
		udi_ubit32_t dev_index, udi_ubit32_t att_index,
				 char *namestr);

char *
_udi_osdep_sprops_get_readfile (_udi_driver_t *drv, udi_ubit32_t index);

udi_sbit8_t *
_osdep_sprops_get_locale (_udi_driver_t *drv, udi_ubit32_t index);

udi_ubit32_t
_osdep_sprops_get_ilocale (_udi_driver_t *drv, udi_ubit32_t index);

udi_ubit32_t
_osdep_sprops_get_nlocale (_udi_driver_t *drv, udi_ubit32_t index);

udi_ubit32_t
_osdep_sprops_get_msg_idx (_udi_driver_t *drv, udi_ubit32_t loc_index,
		udi_ubit32_t str_index);

udi_ubit8_t *
_osdep_sprops_get_msg (_udi_driver_t *drv, udi_ubit32_t loc_index,
		udi_ubit32_t str_index);

char *
_udi_osdep_sprops_ifind_message(_udi_driver_t *drv, char *locale,
		udi_ubit32_t num);

udi_ubit8_t *
_osdep_sprops_afind_message(_udi_driver_t *drv, char *locale,
		char *msgnum);

#if 0
void
_udi_osdep_sprops_get_dev_attr_value(_udi_driver_t *drv, char *name,
		udi_ubit32_t dev_index, udi_instance_attr_type_t attr_type,
				 void *valuep);
#endif

#endif UDI_SPROPS_H
