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

/* #define DEBUG_OSDEP */

#include <udi_env.h>

struct instance_attribute_entry {
	char				*attr;
	unsigned			type;
	char				*value;
};

struct instance_info {
	unsigned			busnum;
	unsigned			slotnum;
	unsigned			funcnum;
	struct instance_attribute_entry	*attr_entry;
};

struct instance_table {
	struct instance_info		**instance_info;
	struct instance_table		*next;
};


/*
 * Initialize the node struct OS dependent part 
 */
void
_udi_dev_node_init(
_udi_driver_t	*driver,
_udi_dev_node_t	*udi_node
)
{
	_OSDEP_DEV_NODE_T		*osinfo;
	udi_instance_attr_type_t	attr_type;
	udi_size_t			length;
	char				bus_type[33];
	udi_ubit32_t			unit_address;

	osinfo = &udi_node->node_osinfo;

	if (driver == NULL) {
		return;
	}

	length = _udi_instance_attr_get(udi_node, "bus_type", bus_type,
		sizeof(bus_type), &attr_type);

	if (length == 0)
	{
		return;
	}

	/*
	 * Handle the PCI case
	 */
	if (!kstrcmp(bus_type, "pci")) 
	{
		_udi_instance_attr_get(udi_node,
			"pci_unit_address", &unit_address,
			sizeof(udi_ubit32_t), &attr_type);

#ifdef DEBUG_OSDEP
		/* Save bridge mapper OS info in the device node */
		udi_debug_printf("get_osinfo:  info=%x node=%x\n",
			osinfo, udi_node);

		udi_debug_printf("dev_node_size=%d osdep_dev_node size=%d\n",
			sizeof(_udi_dev_node_t), sizeof(_OSDEP_DEV_NODE_T));
#endif

		osinfo->devinfo.busnum = (unit_address >> 8) & 0xff;
		osinfo->devinfo.funcnum = unit_address & 0x07;
		osinfo->devinfo.slotnum = (unit_address >> 3) & 0x1f;

		return;
	}
	else if (!strcmp(bus_type,"isa")) 
	{
	}
	else if (!strcmp(bus_type,"eisa")) 
	{
	}
	else if (!strcmp(bus_type,"system")) 
	{
	}

#ifdef DEBUG_OSDEP
	udi_debug_printf("DEV_NODE_INIT: driver=%s node=%x\n",
		driver->drv_name, udi_node);
#endif

	return; 
}

struct instance_table	*_udi_osdep_instance_table_list;

/*
 * Lookup and return an instance attribute from backing store.
 */
udi_size_t
_udi_osdep_instance_attr_get(_udi_dev_node_t *node,
			const char *name,
			void *value,
			udi_size_t length,
			udi_ubit8_t *type)

{
	struct pci_devinfo		*devinfop;
	struct instance_info		**iinfop;
	struct instance_attribute_entry	*ientryp;
	struct instance_table		*itablep;

	*type = UDI_ATTR_NONE;

	devinfop = &node->node_osinfo.devinfo;

#ifdef DEBUG_OSDEP
	udi_debug_printf("_udi_osdep_instance_attr_get: "
		"slot 0x%x, funcnum 0x%x, busnum 0x%x\n",
		devinfop->slotnum,
		devinfop->funcnum,
		devinfop->busnum);
#endif

	ientryp = NULL;

	for (itablep = _udi_osdep_instance_table_list ; itablep != NULL ; 
		itablep = itablep->next)
	{
		for (iinfop = itablep->instance_info ; *iinfop != NULL ;
			iinfop++)
		{
#ifdef DEBUG_OSDEP
			udi_debug_printf("\tchecking: "
				"slot 0x%x, funcnum 0x%x, busnum 0x%x\n",
				(*iinfop)->slotnum,
				(*iinfop)->funcnum,
				(*iinfop)->busnum);
#endif

			if ((*iinfop)->busnum != devinfop->busnum)
			{
				continue;
			}

			if ((*iinfop)->slotnum != devinfop->slotnum)
			{
				continue;
			}

			if ((*iinfop)->funcnum != devinfop->funcnum)
			{
				continue;
			}

#ifdef DEBUG_OSDEP
			udi_debug_printf("\tfound match!\n");
#endif

			for (ientryp = (*iinfop)->attr_entry ; 
				ientryp->attr != NULL ; ientryp++)
			{
#ifdef DEBUG_OSDEP
				udi_debug_printf("\t\tchecking: %s %d "
					"against %s\n",
					ientryp->attr, ientryp->type,
					name + 1);
#endif

				if (strcmp(ientryp->attr, name + 1) != 0)
				{
					continue;
				}

#ifdef DEBUG_OSDEP
				udi_debug_printf("\t\tfound match!\n");
#endif

				break;
			}
		}
	}

	/*
	 * If we didn't find a match, then we are done.
	 */
	if ((ientryp == NULL) || (ientryp->attr == NULL))
	{
#ifdef DEBUG_OSDEP
		udi_debug_printf("\t\tno match!\n");
#endif
		return 0;
	}

	switch (ientryp->type)
	{
	case UDI_ATTR_UBIT32:
		{
			udi_ubit32_t *tmpval = (udi_ubit32_t *)value;

			*tmpval = (udi_ubit32_t)ientryp->value;
		}
		break;

	case UDI_ATTR_BOOLEAN:
		{
			udi_ubit8_t *tmpval = (udi_ubit8_t *) value;

			if (ientryp->value[0] == 'F')
			{
				*tmpval = FALSE;
			}
			else if (ientryp->value[0] == 'T')
			{
				*tmpval = TRUE;
			}
		}
		break;

	case UDI_ATTR_STRING:
		{
			char *tmpval = (char *) value;

			udi_strcpy(tmpval, ientryp->value);
		}
		break;

	default:
		/*
		 * We don't know what this is
		 */
		return 0;
	}

	*type = ientryp->type;

	return length;
}

/*
 * Store a persistent attribute in backing store.
 * FIXME: Not yet implemented.
 */

udi_status_t
_udi_osdep_instance_attr_set(_udi_dev_node_t *node,
			const char *name,
			const void *value,
			udi_size_t length,
			udi_ubit8_t type)
{
	return UDI_OK;
}


void
_udi_register_instance_attributes(
struct instance_table	*itablep
)
{
	itablep->next = _udi_osdep_instance_table_list;

	_udi_osdep_instance_table_list =  itablep;
}

udi_sbit32_t
_udi_get_instance_index(
struct pci_devinfo	*devinfop
)
{
	struct instance_info		**iinfop;
	struct instance_table		*itablep;
	udi_sbit32_t			i;

#ifdef DEBUG_OSDEP
	udi_debug_printf("_udi_get_instance_index: "
		"slot 0x%x, funcnum 0x%x, busnum 0x%x\n",
		devinfop->slotnum,
		devinfop->funcnum,
		devinfop->busnum);
#endif

	for (itablep = _udi_osdep_instance_table_list ; itablep != NULL ; 
		itablep = itablep->next)
	{
		for (i = 0 , iinfop = itablep->instance_info ; *iinfop != NULL ;
			iinfop++, i++)
		{
#ifdef DEBUG_OSDEP
			udi_debug_printf("\tchecking: "
				"slot 0x%x, funcnum 0x%x, busnum 0x%x\n",
				(*iinfop)->slotnum,
				(*iinfop)->funcnum,
				(*iinfop)->busnum);
#endif

			if ((*iinfop)->busnum != devinfop->busnum)
			{
				continue;
			}

			if ((*iinfop)->slotnum != devinfop->slotnum)
			{
				continue;
			}

			if ((*iinfop)->funcnum != devinfop->funcnum)
			{
				continue;
			}

#ifdef DEBUG_OSDEP
			udi_debug_printf("\tfound match! %d\n", i);
#endif

			return i;
		}
	}

	return -1;
}
