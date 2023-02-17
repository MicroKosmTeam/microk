/*
 * File: mapper/linux/udi_bridge_linux.c
 *
 * This registers Linux PCI devices as UDI accessible data
 * structures.
 * It also handles Interrupt abstraction.
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

#define	BRIDGE_MAPPER_VENDOR_ID	0x434D
#define	BRIDGE_MAPPER_CMOS_ID   0x4F44

#include "udi_osdep_opaquedefs.h"
#include <udi_bridge_linux.h>
#include <udi_env.h>
#include <udi_bridgeP.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/config.h> /* has CONFIG_PCI in it */
#include <linux/pci.h>

extern bridge_mapper_child_data_t *
bridge_mapper_find_child_data(bridge_mapper_region_data_t *, udi_ubit32_t);
 
STATIC void
_osdep_bus_bridge_bind_req(udi_bus_bind_cb_t *cb)
{

	/*
	 * Set the pci_dev in the dev_node to the one enumerated by the
	 * bridge mapper for that device
	 */
	bridge_mapper_child_context_t *ccontext = UDI_GCB(cb)->context;
	_udi_dev_node_t *dev_node = _UDI_GCB_TO_CHAN(UDI_GCB(cb))->other_end->
				    chan_region->reg_node;
	bridge_mapper_child_data_t *cdata = 
		bridge_mapper_find_child_data(ccontext->chan_context.rdata, 
					      ccontext->chan_context.child_ID);
	dev_node->node_osinfo.node_devinfo = cdata->osinfo.bridge_child_pci_dev;
}

/* Right now, UDI only supports PCI bindings... so this
 * function walks the kernel PCI device list.
 */
STATIC
int
bridgemap_find_device(struct pci_dev *dev,
        bridge_mapper_child_data_t *cdata)
{
#ifdef BRIDGE_DEBUG
	_OSDEP_PRINTF("bridgemap_find_device\n");
#endif
#ifdef CONFIG_PCI
{
        unsigned short vendor_id, device_id, svendor_id, sdevice_id;
        unsigned char sub_class, base_class, prog_intf, revision_id;
        unsigned long devconf,busnum,funcnum,devnum;

	if (!pci_present()) {
		_OSDEP_PRINTF("udi: pci bus interface not present\n");
		return 0; /* cannot probe the PCI bus */
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
	if (dev == NULL)
		dev = pci_devices;
	else
		dev = dev->next;
#else
	if (dev == NULL)
		dev = pci_dev_g(pci_devices.next);
	else
		dev = pci_dev_g(dev->global_list.next);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
	if (dev == NULL)
#else
	if (dev == pci_dev_g(&pci_devices))
#endif
	{
		/* No more devices. */
		udi_memset(&cdata->osinfo, 0, sizeof(cdata->osinfo));
		return 0;
	}
	else
	{
		/* Prune out all the info the user wants */
		/* To be most generic, we read configuration memory
		 * instead of the pci_dev structure itself.
		 * The functions are in x86 architecture terms:
		 * byte = 8 bits, word = 16 bits, dword = 32 bits */
		if (    pci_read_config_word(dev, PCI_VENDOR_ID_OFFSET,
				&vendor_id) != PCIBIOS_SUCCESSFUL ||
			pci_read_config_word(dev, PCI_DEVICE_ID_OFFSET,
				&device_id) != PCIBIOS_SUCCESSFUL ||
			pci_read_config_byte(dev, PCI_REVISION_ID_OFFSET,
				&revision_id) != PCIBIOS_SUCCESSFUL ||
			pci_read_config_byte(dev, PCI_BASE_CLASS_OFFSET,
				&base_class) != PCIBIOS_SUCCESSFUL ||
			pci_read_config_byte(dev, PCI_SUB_CLASS_OFFSET,
				&sub_class) != PCIBIOS_SUCCESSFUL ||
			pci_read_config_byte(dev, PCI_PROG_INTF_OFFSET,
				&prog_intf) != PCIBIOS_SUCCESSFUL ||
			pci_read_config_word(dev, PCI_SVEN_ID_OFFSET,
				&svendor_id) != PCIBIOS_SUCCESSFUL ||
			pci_read_config_word(dev, PCI_SDEV_ID_OFFSET,
				&sdevice_id) != PCIBIOS_SUCCESSFUL) {
			return 0;
		}
#ifdef DEBUG
		_OSDEP_PRINTF("bridgemap: vendor_id: %04X, device_id: %04X\n", vendor_id, device_id);
		_OSDEP_PRINTF("bridgemap: svendor_id: %04X, sdevice_id: %04X\n", svendor_id, sdevice_id);
#endif
                cdata->pci_vendor_id = vendor_id;
                cdata->pci_device_id = device_id;
                cdata->pci_base_class = base_class;
                cdata->pci_sub_class = sub_class;
                cdata->pci_prog_if = prog_intf;
		cdata->pci_revision_id = revision_id;
		cdata->pci_subsystem_vendor_id = svendor_id;
		cdata->pci_subsystem_id = sdevice_id;

		/* Linux2.2.x:pci.h:devfn: function:3, devicenum:5, busnum:8 */
		busnum = (dev->devfn >> 8) & 0x0F; /* 8 bits */
		devnum = (dev->devfn >> 3) & 0x1F; /* 5 bits */
		funcnum = (dev->devfn) & 0x7; /* 3 bits */
		devconf = (busnum << 8) | (devnum << 3) | funcnum;
		cdata->pci_unit_address = devconf;

		cdata->pci_slot = 0; /* 8 bit slot, if known. 0 otherwise. */

		cdata->osinfo.bridge_child_pci_dev = dev;

		return 1;
	}
}
#else
#warning This machine does not have Kernel PCI enabled.
	return 0;
#endif
}

/*
 * Dummy up a PCI entry for the CMOS driver
 */
STATIC
void
bridge_mapper_fake_cmos_device( bridge_mapper_child_data_t *cdata)
{
#ifdef BRIDGE_DEBUG
	_OSDEP_PRINTF("bridge_mapper_fake_cmos_device\n");
#endif
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
	cdata->osinfo.bridge_child_pci_dev = NULL;
}


/****************************************/
/*	_OSDEP_ENUMERATE_PCI_DEVICE	*/
/****************************************/
udi_boolean_t
bridgemap_enumerate_pci_device( bridge_mapper_child_data_t *cdata,
				  udi_enumerate_cb_t *cb,
				  udi_ubit8_t enumeration_level)
{
	static struct pci_dev* osdep_key = NULL;

#ifdef BRIDGE_DEBUG
	_OSDEP_PRINTF("bridgemap_enumerate_pci_device\n");
#endif
	switch (enumeration_level) {
		case UDI_ENUMERATE_START:
		case UDI_ENUMERATE_START_RESCAN:
			/************************/
			/* start from beginning */
			/************************/
			bridge_mapper_fake_cmos_device(cdata);
			osdep_key = NULL;
			return TRUE;
			break;
		case UDI_ENUMERATE_NEXT:
			/************************/
			/* get next             */
			/************************/
			break;
		case UDI_ENUMERATE_DIRECTED:
		case UDI_ENUMERATE_NEW:
		default:
			/* Unknown option assert for now. */
			_OSDEP_ASSERT(0 && "Unimplemented bridge enum option.");

	}

        if (bridgemap_find_device( osdep_key, cdata)) {
                osdep_key = cdata->osinfo.bridge_child_pci_dev; /* save a key */
                return TRUE;
        }

	return FALSE;
}

extern int _udi_debug_isr;
int _udi_debug_isr = 0;

/*
 * The Linux Interrupt Handler
 */

STATIC _OSDEP_ISR_RETURN_T _udi_isr0(int irq, void *dev_id, struct pt_regs *regs);


udi_status_t
_udi_register_isr(_OSDEP_BRIDGE_CHILD_DATA_T * dpp, _OSDEP_ISR_RETURN_T (*isr) (void*),
		  void *context, _udi_intr_info_t * osinfo)
{
    int irq;
    struct pci_dev *dp = dpp->bridge_child_pci_dev;
    udi_boolean_t is_shared;
    /*TBD: udi_instance_attr_type_t shared_type;*/
    intr_attach_context_t* icontext=(intr_attach_context_t*)context;
    const _udi_region_t* intr_region = 
	((_udi_channel_t*)icontext->intr_event_channel)->other_end->chan_region;
    const _udi_driver_t* driver = intr_region->reg_driver;
    const char* drv_name= driver->drv_name;
#ifdef BRIDGE_DEBUG
   _OSDEP_PRINTF("_udi_register_isr dpp=%p isr=%p\n", dpp, isr);
#endif
    if (dp == NULL)
	return UDI_STAT_MISTAKEN_IDENTITY;

    /*
     * Find out if this interrupt can be shared
     * (FIXME) I think the nonsharable_interrupt declaration was forgotten
     * when the sprops structure was designed. Even though it is in the 
     * udiprops.txt file, it doesn't get stored in the driver's sprops struct.
     * Thus, I cannot get it here. This needs to be fixed!
     */
#if 0
    is_shared = _osdep_sprops_get_non_sharable_isr_attr(driver,
							icontext->interrupt_idx); 
#endif
	is_shared = TRUE;
    
    irq = dp->irq;

    osinfo->isr_func = isr;

    osinfo->context = context;

    /* context is of type intr_attach_context_t */

    osinfo->irq = irq;

    if (request_irq(irq, _udi_isr0, is_shared ? SA_SHIRQ : 0, drv_name, osinfo))
	_OSDEP_PRINTF("_udi_register_isr: request_irq %d failed\n", irq);
    if (_udi_debug_isr > 0)
	_OSDEP_PRINTF
	    ("_udi_register_isr:irq %d, isr 0x%p, context 0x%p, osinfo 0x%p\n",
	     irq, isr, context, osinfo);
    return UDI_OK;
}

udi_status_t
_udi_unregister_isr(_OSDEP_BRIDGE_CHILD_DATA_T * cd, void (*isr) (), 
		    void *context, _udi_intr_info_t * osinfo)
{
#ifdef BRIDGE_DEBUG
    _OSDEP_PRINTF("_udi_unregister_isr cd=%p, isr=%p, context=%p, osinfo=%p\n",
		  cd, isr, context, osinfo);
#endif
    free_irq(osinfo->irq, osinfo);
    return UDI_OK;
}


/*
 * This needs to stay in here as Linux interrupt handlers take a different
 * set of parameters than UDI ones and we have to translate inbetween them
 */

STATIC _OSDEP_ISR_RETURN_T _udi_isr0(int irq, void *dev_id, 
				     struct pt_regs *regs)
{
    _udi_intr_info_t *osinfo = (_udi_intr_info_t *) dev_id;

    if (_udi_debug_isr > 2)
	_OSDEP_PRINTF("_udi_isr0:irq %d, handler 0x%p, context 0x%p\n",
		      irq, osinfo->isr_func, osinfo->context);
    (*osinfo->isr_func) (osinfo->context);
    return;
}

STATIC void
mapper_init()
{
#ifdef BRIDGE_DEBUG
   _OSDEP_PRINTF("mapper_init: bridge\n");
#endif
}

STATIC void
mapper_deinit()
{
#ifdef BRIDGE_DEBUG
   _OSDEP_PRINTF("mapper_deinit: bridge\n");
#endif
}

#include "bridgecommon/udi_bridge.c"
