
/*
 * File: mapper/posix/bridge/bridgemap.c
 *
 * Posix test-frame bridge mapper
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

/* UDI includes */

#define	UDI_VERSION		0x101	/* 1.0 conformance */
#define	UDI_PHYSIO_VERSION	0x101

/* Almost Exclusively Common Code */

#define _OSDEP_BUS_BRIDGE_DEINIT(x) posix_bridgemap_deinit()
extern void posix_bridgemap_deinit();

#include "udi_bridge.c"

#if _UDI_PHYSIO_SUPPORTED

/****************************************/

/*	Generate some fake devices.	*/

/****************************************/

posix_pci_device_item_t faked_device_list[] = {
	{1, 0x1111, 0x1111, 0x11, 0x11, 0x11, 0x11, 0x1111, 0x1111,
	 0x1111, 0x11},			/* Fake device 1 */
	{1, 0x1234, 0x1234, 0x12, 0x12, 0x12, 0x12, 0x1234, 0x1234,
	 0x1234, 0x12},			/* Fake device 2 */
	{1, 0x434d, 0x4f44, 0x00, 0x00, 0x00, 0x00, 0x0000, 0x0000,
	 0xFFFF, 0xFF}			/* CMOS device */
};

posix_pci_device_item_t *device_list = NULL;
posix_pci_device_item_t *device_ptr = NULL;

STATIC void posix_populate_device_list(void);
STATIC int posix_do_device(bridge_mapper_child_data_t * x);
STATIC int posix_force_device(bridge_mapper_child_data_t * x,
			      udi_instance_attr_list_t *attr_list,
			      udi_size_t attr_list_len);


STATIC void
posix_populate_device_list(void)
{
	int num_faked_devs, num_devices, device_list_size;
	void *memptr;

	num_faked_devs =
		sizeof (faked_device_list) / sizeof (posix_pci_device_item_t);
	num_devices = POSIXUSER_NUM_PCI_DEVICES();
	device_list_size =
		sizeof (posix_pci_device_item_t) * (1 + num_faked_devs +
						    num_devices);

	device_list = memptr = malloc(device_list_size);
	memset(device_list, 0, device_list_size);

	/*
	 * Copy in the fake Posix test-frame harness devices. 
	 */
	memcpy(device_list, faked_device_list, sizeof (faked_device_list));
	device_list += num_faked_devs;

	/*
	 * Insert os-dependent (possibly real) devices. 
	 */
	POSIXUSER_ENUMERATE_PCI_DEVICES(device_list);
	device_list += num_devices;

	/*
	 * last element in the list must be all zeros. 
	 */
	memset(device_list, 0, sizeof (posix_pci_device_item_t));
	device_list = (posix_pci_device_item_t *) memptr;
}


extern void
posix_bridgemap_deinit()
{
	if (device_list) {
		free(device_list);
		device_list = NULL;
		device_ptr = NULL;
	}
}


/***********************************/

/* this fakes the enumeration code */

/***********************************/
STATIC int
posix_do_device(bridge_mapper_child_data_t * x)
{
	if (!device_ptr->device_present)
		return (0);

	x->pci_vendor_id = device_ptr->pci_vendor_id;
	x->pci_device_id = device_ptr->pci_device_id;
	x->pci_revision_id = device_ptr->pci_revision_id;
	x->pci_base_class = device_ptr->pci_base_class;
	x->pci_sub_class = device_ptr->pci_sub_class;
	x->pci_prog_if = device_ptr->pci_prog_if;
	x->pci_subsystem_vendor_id = device_ptr->pci_subsystem_vendor_id;
	x->pci_subsystem_id = device_ptr->pci_subsystem_id;
	x->pci_unit_address = device_ptr->pci_unit_address;
	x->pci_slot = device_ptr->pci_slot;

	if (device_ptr->device_present)
		device_ptr++;

	return (1);
}

STATIC int
posix_force_device(bridge_mapper_child_data_t * x,
		   udi_instance_attr_list_t *attr_list,
		   udi_size_t attr_list_len)
{
	int ii;

	if (!attr_list)
		return (0);

	x->pci_vendor_id = 0;
	x->pci_device_id = 0;
	x->pci_revision_id = 0;
	x->pci_base_class = 0;
	x->pci_sub_class = 0;
	x->pci_prog_if = 0;
	x->pci_subsystem_vendor_id = 0;
	x->pci_subsystem_id = 0;
	x->pci_unit_address = 0;
	x->pci_slot = 0;

	for (ii = 0; ii < attr_list_len; ii++) {

#define pci_set(Field)							\
    else if (!strncmp(attr_list[ii].attr_name, "pci_" #Field,		\
		      sizeof(attr_list[ii].attr_name))) {		\
	x->pci_##Field = UDI_ATTR32_GET(attr_list[ii].attr_value);	\
        attr_list[ii].attr_name[0] = 0;  /* allow overwrite */		\
    }
#define pci_baseclass pci_base_class

		if (!strncmp(attr_list[ii].attr_name, "bus_type",
			     sizeof (attr_list[ii].attr_name))) {
			if (strncmp
			    ((const char *)attr_list[ii].attr_value, "pci",
			     sizeof (attr_list[ii].attr_value))) {
				printf("----PCI Bridge: tried to force enumeration of non-PCI device (%s)!\n", attr_list[ii].attr_value);
				return (0);
			}
			/*
			 * nothing to do for this attribute... 
			 */
		}
		pci_set(vendor_id)
			pci_set(device_id)
			pci_set(revision_id)
			pci_set(baseclass)
			pci_set(sub_class)
			pci_set(prog_if)
			pci_set(subsystem_vendor_id)
			pci_set(subsystem_id)
			pci_set(unit_address)
			pci_set(slot);
		/*
		 * else ignored 
		 */
	}

#undef pci_set

	return (1);
}

/****************************************/

/*	_OSDEP_ENUMERATE_PCI_DEVICE	*/

/****************************************/
udi_boolean_t
_posix_stub_enumerate_pci_device(bridge_mapper_child_data_t * rdata,
				 udi_enumerate_cb_t *cb,
				 udi_ubit8_t enumeration_level)
{
	DEBUG4(4050, 0, (0));

	/*
	 * just look for valid input 
	 */
	if (device_list == NULL) {
		posix_populate_device_list();
	}

	switch (enumeration_level) {
	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:

			/************************/
		/*
		 * start from beginning 
		 */

			/************************/
		device_ptr = &device_list[0];
		DEBUG4(4051, 0, (0));
		break;
	case UDI_ENUMERATE_NEXT:

			/************************/
		/*
		 * get next             
		 */

			/************************/
		DEBUG4(4052, 0, (0));
		break;
	case UDI_ENUMERATE_DIRECTED:

			/*****************************/
		/*
		 * return what we were given 
		 */

			/*****************************/
		return (posix_force_device(rdata, cb->attr_list,
					   cb->attr_valid_length));
	case UDI_ENUMERATE_NEW:
	default:
		/*
		 * Unknown option assert for now. 
		 */
		_OSDEP_ASSERT(0);

	}

	return (posix_do_device(rdata));
}

struct registry {
	_OSDEP_ISR_RETURN_T(*isr) (void *);
	_posix_intr_osinfo_t *attach_osinfo;
} _posix_stub_registry[] = {
	{
	0, 0}, {
	0, 0}, {
	0, 0}, {
	0, 0}
};

int device_count = 0;


udi_status_t
_posix_stub_register_isr(_OSDEP_ISR_RETURN_T(*isr) (void *),
			 udi_cb_t *gcb,
			 void *context,
			 void *enumeration_osinfo,
			 void *attach_osinfo)
{

	printf("_posix_stub_register_isr isr %p\n", isr);
	printf("_posix_stub_register_isr gcb %p\n", gcb);
	printf("_posix_stub_register_isr context %p\n", context);
	printf("_posix_stub_register_isr attach_osinfo %p\n", attach_osinfo);

	/*
	 * cross connect the osinfo and the interrupt context 
	 */
	((_posix_intr_osinfo_t *) attach_osinfo)->udi_attach_info = context;
	((_posix_intr_osinfo_t *) attach_osinfo)->isr = isr;

	_posix_stub_registry[device_count].isr = isr;
	_posix_stub_registry[device_count].attach_osinfo = attach_osinfo;
	device_count++;

	printf("_posix_stub_register_isr %p\n", isr);
	return (UDI_OK);
}

udi_status_t
_posix_stub_unregister_isr(_OSDEP_ISR_RETURN_T(*isr) (void *),
			   udi_cb_t *gcb,
			   void *context,
			   void *enumeration_osinfo,
			   void *attach_osinfo)
{
	return (UDI_OK);
}

void
_posix_stub_isr_acknowledge(udi_ubit8_t intr_status)
{
}

void *
_posix_stub_get_isr_context(void *attach_osinfo)
{
	return ((void *)(((_posix_intr_osinfo_t *) attach_osinfo)->
			 udi_attach_info));
}

void *
_posix_stub_get_event_info(void *_context)
{
	return ((void *)(NULL));
}


_OSDEP_ISR_RETURN_T
_posix_stub_isr_return_val(udi_ubit8_t intr_status)
{
	/*
	 * convert intr_status into something the OS likes.... 
	 */
	return ((_OSDEP_ISR_RETURN_T) intr_status);
}




/* exercise the interrupt event channel from the os side.... */
int
posix_OS_interrupt()
{
	int i;
	int intr_count = 0;

/*
 *	Set up an interrupt source and service them....
 */
	while (1) {

		intr_count++;
		for (i = 0; i < device_count; i++) {

			(_posix_stub_registry[i].isr) (_posix_stub_registry[i].
						       attach_osinfo);
		}
		if (intr_count > 10) {
			printf("Ten interrupts serviced.   Success!\n");
			break;
		}
	}

	return 0;
}
#endif /* _UDI_PHYSIO_SUPPORTED */
