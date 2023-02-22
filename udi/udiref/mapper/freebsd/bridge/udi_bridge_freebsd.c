/*
 * File: mapper/uw/bridge/udi_bridge_freebsd.c
 *
 * FreeBSD-specific component of UDI Bridge Mapper.
 *
 * $Copyright udi_reference:
 * 
 * 
 *    Copyright (c) 1995-2000; Compaq Computer Corporation; Hewlett-Packard
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
#define	BRIDGE_MAPPER_CMOS_ID	0x4F44

#include <udi_env.h>			/* Environment specific definitions */
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <machine/bus.h>
#include <machine/resource.h>
#include <sys/rman.h>

udi_status_t
_udi_register_isr(_OSDEP_BRIDGE_CHILD_DATA_T * dpp,
		  _OSDEP_ISR_RETURN_T(*isr) (void *),
		  void *context,
		  _udi_intr_info_t * osinfo)
{
	int irqid = 0;
	udi_boolean_t is_shareable = TRUE;

	/*
	 * FIXME: consult with sprops to see if this is shareable.
	 */
	is_shareable = TRUE;

	dpp->irq = bus_alloc_resource(dpp->device, SYS_RES_IRQ, &irqid,
				      0, ~0, 1,
				      RF_ACTIVE | is_shareable ? RF_SHAREABLE :
				      0);
	if (dpp->irq == NULL) {
		/*
		 * FIXME: log an error 
		 */
		printf("bus_alloc_resource IRQ registration failed\n");
		return UDI_STAT_RESOURCE_UNAVAIL;
	}

	/*
	 * FIXME: figure out what the last "cookiep" argument is all about
	 * since there isn't a man page...
	 */

	if (bus_setup_intr(dpp->device, dpp->irq, INTR_TYPE_MISC,
			   isr, context, &dpp->intrhand)) {
		bus_teardown_intr(dpp->device, dpp->irq, &dpp->intrhand);
		bus_release_resource(dpp->device, SYS_RES_IRQ, irqid, dpp->irq);
		/*
		 * FIXME: log an error 
		 */
		return UDI_STAT_RESOURCE_UNAVAIL;
	}
	return UDI_OK;
}

udi_status_t
_udi_unregister_isr(_OSDEP_BRIDGE_CHILD_DATA_T * dpp,
		    void (*isr) (),
		    void *context,
		    _udi_intr_info_t * osinfo)
{
	bus_teardown_intr(dpp->device, dpp->irq, dpp->intrhand);
	bus_release_resource(dpp->device, SYS_RES_IRQ, 0, dpp->irq);

	return UDI_OK;
}

/*
 * FreeBSD-specific DMA constraints that are more restrictive than
 * the UDI defaults.
 */
#define _OSDEP_BUS_BRIDGE_ALLOC_CONSTRAINTS

udi_dma_constraints_attr_spec_t _osdep_bridge_attr_list[] = {
	{ UDI_DMA_ADDRESSABLE_BITS, 8 * sizeof(vm_offset_t) },
	{ UDI_DMA_SCGTH_MAX_ELEMENTS, 256 },
};

#include "bridgecommon/udi_bridge.c"

/*
 * NOTE NOTE NOTE: HORRIBLE KLUDGE FOLLOWS
 *
 * This is the Wrong Way to do this.  Including private structures out
 * of the kernel's pci subsystem then groping its internal data structures
 * is a bad idea for a hundred reasons, but for proof of concept, I decided
 * this was OK to do until it could be better coordinated with the FreeBSD
 * newbus/nexus/pci code.
 *
 * Humiliate me if you see fit.
 *
 * robertl
 */

typedef struct pcicfg {
	struct device *dev;
	void *hdrspec;
	u_int16_t subvendor;
	u_int16_t subdevice;
	u_int16_t vendor;
	u_int16_t device;
	u_int16_t cmdreg;
	u_int16_t statreg;
	u_int8_t baseclass;
	u_int8_t subclass;
	u_int8_t progif;
	u_int8_t revid;
	u_int8_t hdrtype;
	u_int8_t cachelnsz;
	u_int8_t intpin;
	u_int8_t intline;
	u_int8_t mingnt;
	u_int8_t maxlat;
	u_int8_t lattimer;
	u_int8_t mfdev;
	u_int8_t nummaps;
	u_int8_t hose;
	u_int8_t bus;
	u_int8_t slot;
	u_int8_t func;
	u_int8_t secondarybus;
	u_int8_t subordinatebus;
} pcicfgregs;

struct pcisel {
	u_int8_t pc_bus;
	u_int8_t pc_dev;
	u_int8_t pc_func;
};

struct pci_conf {
	struct pcisel pc_sel;
	u_int8_t pc_hdr;
	u_int16_t pc_subvendor;
	u_int16_t pc_subdevice;
	u_int16_t pc_vendor;
	u_int16_t pc_device;
	u_int8_t pc_class;
	u_int8_t pc_subclass;
	u_int8_t pc_progif;
	u_int8_t pc_revid;
	char pd_name[17];
	u_long pd_unit;
};

struct pci_devinfo {
	struct {
		struct pci_devinfo *stqe_next;
	} pci_links;
	struct resource_list resources;
	pcicfgregs cfg;
	struct pci_conf conf;
};




/*
 * Dummy up a PCI entry for the CMOS driver
 */
STATIC void
bridge_mapper_fake_cmos_device(bridge_mapper_child_data_t * cdata)
{
	uprintf("Fake cmos\n");
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
}

STATIC int
bridgemap_find_device(struct pci_devinfo *dinfo,
		      bridge_mapper_child_data_t * cdata)
{

#if 0
	printf("(struct pci_devinfo *)%p ", dinfo);
#endif
	if (dinfo == NULL) {
		return 0;
	}

	cdata->pci_vendor_id = dinfo->cfg.vendor;
	cdata->pci_device_id = dinfo->cfg.device;
	cdata->pci_revision_id = dinfo->cfg.revid;
	cdata->pci_base_class = dinfo->cfg.baseclass;
	cdata->pci_sub_class = dinfo->cfg.subclass;
	cdata->pci_prog_if = dinfo->cfg.progif;
	cdata->pci_subsystem_vendor_id = dinfo->cfg.subvendor;
	cdata->pci_subsystem_id = dinfo->cfg.subdevice;
#if 0
	cdata->pci_unit_address = devconf;
#endif
	cdata->pci_slot = dinfo->cfg.slot;

	printf("Adding %04x/%04x/%04x. %p\n", cdata->pci_vendor_id,
	       cdata->pci_device_id, cdata->pci_revision_id, dinfo->cfg.dev);

	cdata->osinfo.device = dinfo->cfg.dev;

	return 1;
}


/*
 *	_OSDEP_ENUMERATE_PCI_DEVICE
 */
udi_boolean_t
bridgemap_enumerate_pci_device(bridge_mapper_child_data_t * cdata,
			       udi_enumerate_cb_t *cb,
			       udi_ubit8_t enumeration_level)
{
	static struct pci_devinfo *dinfo = NULL;
	struct devlist *devlist_head;
	extern STAILQ_HEAD(devlist,
			   pci_devinfo) pci_devq;

	switch (enumeration_level) {
	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:
		udi_debug_printf("UDI_ENUMERATE_START or "
				 "UDI_ENUMERATE_START_RESCAN\n");

		/*
		 * start from beginning
		 */

		devlist_head = &pci_devq;
		dinfo = STAILQ_FIRST(devlist_head);

		bridge_mapper_fake_cmos_device(cdata);

		return TRUE;

	case UDI_ENUMERATE_NEXT:
		/*
		 * get next
		 */
		break;

	case UDI_ENUMERATE_DIRECTED:
	case UDI_ENUMERATE_NEW:
	default:

		/*
		 * Unknown option assert for now.
		 */
		_OSDEP_ASSERT(0);
	}

	if (bridgemap_find_device(dinfo, cdata)) {
		/*
		 * On next entry, pick up next list.
		 */
		dinfo = STAILQ_NEXT(dinfo, pci_links);
		return TRUE;
	} else {
		/*
		 * No more to enumerate.
		 */
		dinfo = NULL;
		return FALSE;
	}
}

static int
_udi_bridge_mapper_mod_event(module_t mod,
			     int what,
			     void *arg)
{
	switch (what) {
	case MOD_LOAD:
		break;

	case MOD_UNLOAD:
		break;
	}

	return 0;

}


DEV_MODULE(udi_bridge_mapper, _udi_bridge_mapper_mod_event, NULL);
