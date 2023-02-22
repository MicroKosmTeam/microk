/*
 * File: mapper/uw/bridge/udi_bridge_uw.c
 *
 * UnixWare-specific component of UDI Bridge Mapper.
 *
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

#define	BRIDGE_MAPPER_VENDOR_ID	0x434D
#define	BRIDGE_MAPPER_CMOS_ID   0x4F44

#include "bridgecommon/udi_bridge.c"

/*
 * External dev_info_t queue access [env/solaris/udi_osdep.c]
 */
extern udi_boolean_t	_udi_dev_info_match(void **, udi_instance_attr_list_t *,
					    udi_ubit8_t);

#if 0
/* Generic Configuration Space register offsets */
#define PCI_VENDOR_ID_OFFSET		0x00
#define PCI_DEVICE_ID_OFFSET		0x02
#define PCI_COMMAND_OFFSET		0x04
#define PCI_REVISION_ID_OFFSET		0x08
#define PCI_PROG_INTF_OFFSET		0x09
#define PCI_SUB_CLASS_OFFSET		0x0A
#define PCI_BASE_CLASS_OFFSET		0x0B
#define PCI_CACHE_LINE_SIZE_OFFSET	0x0C
#define PCI_LATENCY_TIMER_OFFSET	0x0D
#define PCI_HEADER_TYPE_OFFSET		0x0E
#define PCI_BIST_OFFSET			0x0F
/* Ordinary ("Type 0") Device Configuration Space register offsets */
#define	PCI_BASE_REGISTER_OFFSET    	0x10
#define PCI_SVEN_ID_OFFSET	    	0x2C
#define PCI_SDEV_ID_OFFSET	    	0x2E
#define PCI_E_ROM_BASE_ADDR_OFFSET  	0x30
#define PCI_INTERPT_LINE_OFFSET     	0x3C
#define PCI_INTERPT_PIN_OFFSET      	0x3D
#define PCI_MIN_GNT_OFFSET	    	0x3E
#define PCI_MAX_LAT_OFFSET	    	0x3F

/* PCI-to-PCI Bridge ("Type 1") Configuration Space register offsets */
#define	PCI_PPB_SECONDARY_BUS_NUM_OFFSET	0x19
#define	PCI_PPB_SUBORDINATE_BUS_NUM_OFFSET	0x1A
#endif

#if 0 || UNIXWARE
static
int
bridgemap_find_device(rm_key_t rm_key,
	bridge_mapper_child_data_t *cdata)
{
	struct rm_args rma;
	cm_args_t cma;
	cm_num_t bus, pci_bus = CM_BUS_PCI;
	ushort_t vendor_id, device_id, svendor_id, sdevice_id;
	uchar_t sub_class, base_class, prog_intf, revision_id;
	cm_num_t slot, devconf,busnum,funcnum,devnum;

	/* 
	 *  This is the (sole?) reason we are $interface base.
	 *  this function isn't in DDI.
	 */
	extern int
	cm_find_match(struct rm_args * rma, void * val, size_t vallen);

	rma.rm_key = rm_key;
	strcpy(rma.rm_param, CM_BRDBUSTYPE);
	rma.rm_vallen = sizeof(cm_num_t);
	rma.rm_val = &bus;
	rma.rm_n = 0;

	if (cm_find_match(&rma, &pci_bus, sizeof(cm_num_t)) == -1)
		return 0;	/* no pci device found */

	/*
	 * Get the PCI information
	 */

	cm_begin_trans(rma.rm_key, RM_READ);

	cma.cm_key = rma.rm_key;
	cma.cm_param = CM_SLOT;
	cma.cm_val = &slot;
	cma.cm_vallen = sizeof(cm_num_t);
	cma.cm_n = 0;


	/* you may *NOT* have a slot number - new to PCI 2.1*/
	(void)cm_getval(&cma);
	cm_end_trans(rma.rm_key);

	/*
	 * Get the bus,dev,func values
	 */
	cm_begin_trans(rma.rm_key, RM_READ);
	cma.cm_param = CM_BUSNUM;
	cma.cm_val = &busnum;
	cma.cm_vallen = sizeof(cm_num_t);
	cma.cm_n = 0;

	if(cm_getval(&cma)!=0){
		printf("bridgemap_find_device: CM_BUSNUM not found\n");
		cm_end_trans(rma.rm_key);
		return 0;
	}
	cm_end_trans(rma.rm_key);


	/*
	 * Get the bus,dev,func values
	 */
	cma.cm_param = CM_DEVNUM;
	cma.cm_val = &devnum;
	cma.cm_vallen = sizeof(cm_num_t);
	cma.cm_n = 0;

	cm_begin_trans(rma.rm_key, RM_READ);
	if(cm_getval(&cma)!=0){
		printf("bridgemap_find_device: CM_DEVNUM not found\n");
		cm_end_trans(rma.rm_key);
		return 0;
	}
	cm_end_trans(rma.rm_key);


	/*
	 * Get the bus,dev,func values
	 */
	cma.cm_param = CM_FUNCNUM;
	cma.cm_val = &funcnum;
	cma.cm_vallen = sizeof(cm_num_t);
	cma.cm_n = 0;


	cm_begin_trans(rma.rm_key, RM_READ);
	if(cm_getval(&cma)!=0){
		printf("bridgemap_find_device: CM_FUNCNUM not found\n");
		cm_end_trans(rma.rm_key);
		return 0;
	}
	cm_end_trans(rma.rm_key);

	devconf = (busnum << 8) | (devnum << 3) | funcnum;

	if ( cm_read_devconfig16(rma.rm_key, PCI_VENDOR_ID_OFFSET,
			&vendor_id) == -1 ||
		cm_read_devconfig16(rma.rm_key, PCI_DEVICE_ID_OFFSET,
			&device_id) == -1 ||
		cm_read_devconfig8(rma.rm_key, PCI_REVISION_ID_OFFSET,
			&revision_id) == -1 ||
		cm_read_devconfig8(rma.rm_key, PCI_BASE_CLASS_OFFSET,
			&base_class) == -1 ||
		cm_read_devconfig8(rma.rm_key, PCI_SUB_CLASS_OFFSET,
			&sub_class) == -1 ||
		cm_read_devconfig8(rma.rm_key, PCI_PROG_INTF_OFFSET,
			&prog_intf) == -1 ||
		cm_read_devconfig16(rma.rm_key, PCI_SVEN_ID_OFFSET,
			&svendor_id) == -1 ||
		cm_read_devconfig16(rma.rm_key, PCI_SDEV_ID_OFFSET,
			&sdevice_id) == -1) {
		return 0;
	}

	cdata->pci_vendor_id = vendor_id;
	cdata->pci_device_id = device_id;
	cdata->pci_revision_id = revision_id;
	cdata->pci_base_class = base_class;
	cdata->pci_sub_class = sub_class;
	cdata->pci_prog_if = prog_intf;
	cdata->pci_subsystem_vendor_id = svendor_id;
	cdata->pci_subsystem_id = sdevice_id;
	cdata->pci_unit_address = devconf;
	cdata->pci_slot = slot;
	cdata->osinfo = rma.rm_key;

	return 1;
}	

/*
 * Dummy up a PCI entry for the CMOS driver
 */
STATIC
void
bridge_mapper_fake_cmos_device( bridge_mapper_child_data_t *cdata)
{
	cdata->pci_vendor_id = BRIDGE_MAPPER_VENDOR_ID;
	cdata->pci_device_id = BRIDGE_MAPPER_CMOS_ID;
	cdata->pci_revision_id = 0;
	cdata->pci_base_class = 0;
	cdata->pci_sub_class = 0;
	cdata->pci_prog_if = 0;
	cdata->pci_subsystem_vendor_id = 0;
	cdata->pci_subsystem_id = 0;
	cdata->pci_unit_address = 0;
	cdata->pci_slot = 0;
	cdata->osinfo = RM_NULL_KEY;
}
#endif	/* 0 || UNIXWARE */

#define	_UDI_SET_CDATA(cb, cdata, name)	\
	_udi_set_cdata((cb), &cdata->name, #name)

/*
 * _udi_set_cdata:
 * --------------
 * Update the bridge mapper's cdata if the <name> attribute is present in the
 * <cb>'s attr_list. Update the cb attribute too
 */
static void
_udi_set_cdata(udi_enumerate_cb_t *cb, udi_ubit32_t *val, char *name)
{
	udi_ubit8_t			n, i;
	udi_instance_attr_list_t	*list;
	udi_ubit32_t			tmpval;

	list = cb->attr_list;
	for (n = 0; n < cb->attr_valid_length; n++, list++) {
		if (!udi_strcmp(name, list->attr_name)) {
			*val = UDI_ATTR32_GET(list->attr_value);
			for (i = n+1; i < cb->attr_valid_length; i++) {
				list[n] = list[i];
				n++;
			}
			cb->attr_valid_length--;
			return;
		}
	}
}

/*
 * This enumeration works only for UDI_ENUMERATE_DIRECTED case as the /devices
 * tree drives the whole mechanism.
 * For hardware drivers [the only sort we're interested in] we obtain all the
 * PCI device attributes and pass them in via udi_enumerate_req(). All other
 * enumeration types will fail
 */

/****************************************/
/*	_OSDEP_ENUMERATE_PCI_DEVICE	*/
/****************************************/
udi_boolean_t
bridgemap_enumerate_pci_device( bridge_mapper_child_data_t *cdata,
				  udi_enumerate_cb_t *cb,
				  udi_ubit8_t enumeration_level)
{
	void		*dip;
	udi_boolean_t	rval;

	/* just look for valid input */

	switch (enumeration_level) {
		case UDI_ENUMERATE_START:
		case UDI_ENUMERATE_START_RESCAN:
		case UDI_ENUMERATE_NEXT:
		case UDI_ENUMERATE_NEW:
			return FALSE;
			break;
		case UDI_ENUMERATE_DIRECTED:
			dip = cdata->osinfo;
			rval = _udi_dev_info_match(&dip, cb->attr_list,
						   cb->attr_valid_length);
			if (rval == FALSE) {
				udi_debug_printf(
				"bridgemap_enumerate_pci_device - NULL dip");
			} else {
				/*
				 * Set the cdata values to those passed in and
				 * update the osinfo field accordingly
				 */
				cdata->osinfo = dip;
				_UDI_SET_CDATA(cb, cdata, pci_vendor_id);
				_UDI_SET_CDATA(cb, cdata, pci_device_id);
				_UDI_SET_CDATA(cb, cdata, pci_revision_id);
				_UDI_SET_CDATA(cb, cdata, pci_base_class);
				_UDI_SET_CDATA(cb, cdata, pci_sub_class);
				_UDI_SET_CDATA(cb, cdata, pci_prog_if);
				_UDI_SET_CDATA(cb, cdata, 
						pci_subsystem_vendor_id);
				_UDI_SET_CDATA(cb, cdata, pci_subsystem_id);
				_UDI_SET_CDATA(cb, cdata, pci_unit_address);
				_UDI_SET_CDATA(cb, cdata, pci_slot);
			}
			return rval;
		default:
			/* Unknown option assert for now. */
			_OSDEP_ASSERT(0);

	}
}

