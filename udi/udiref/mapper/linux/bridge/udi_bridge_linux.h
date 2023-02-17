/*
 * File: udi_bridge_linux.h
 * 
 * Linux bridge mapper header file.
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

#ifndef __UDI_OSDEP_BRIDGE_H__
#define __UDI_OSDEP_BRIDGE_H__

/*
 * Bridge mapper macros, definitions.
 */

#define _OSDEP_INTR_T			    _udi_intr_info_t
#define _OSDEP_BRIDGE_RDATA_T		_osdep_bridge_rdata_t
#define _OSDEP_BRIDGE_CHILD_DATA_T	_osdep_bridge_child_data_t
#define _OSDEP_ENUMERATE_PCI_DEVICE	bridgemap_enumerate_pci_device

#define _OSDEP_REGISTER_ISR(isr, gcb, context, enum_osinfo, attach_osinfo) \
		_udi_register_isr(enum_osinfo, isr, context, attach_osinfo)
#define _OSDEP_UNREGISTER_ISR(isr, gcb, context, enum_osinfo, attach_osinfo) \
		_udi_unregister_isr(enum_osinfo, isr, context, attach_osinfo)


#define	BRIDGE_MAPPER_VENDOR_ID	0x434D
#define	BRIDGE_MAPPER_CMOS_ID   0x4F44

/* Generic PCI Configuration Space register offsets */
#define PCI_VENDOR_ID_OFFSET            0x00
#define PCI_DEVICE_ID_OFFSET            0x02
#define PCI_COMMAND_OFFSET              0x04
#define PCI_REVISION_ID_OFFSET          0x08
#define PCI_PROG_INTF_OFFSET            0x09
#define PCI_SUB_CLASS_OFFSET            0x0A
#define PCI_BASE_CLASS_OFFSET           0x0B
#define PCI_CACHE_LINE_SIZE_OFFSET      0x0C
#define PCI_LATENCY_TIMER_OFFSET        0x0D
#define PCI_HEADER_TYPE_OFFSET          0x0E
#define PCI_BIST_OFFSET                 0x0F
/* Ordinary ("Type 0") Device Configuration Space register offsets */
#define PCI_BASE_REGISTER_OFFSET        0x10
#define PCI_SVEN_ID_OFFSET              0x2C
#define PCI_SDEV_ID_OFFSET              0x2E
#define PCI_E_ROM_BASE_ADDR_OFFSET      0x30
#define PCI_INTERPT_LINE_OFFSET         0x3C
#define PCI_INTERPT_PIN_OFFSET          0x3D
#define PCI_MIN_GNT_OFFSET              0x3E
#define PCI_MAX_LAT_OFFSET              0x3F

/* PCI-to-PCI Bridge ("Type 1") Configuration Space register offsets */
#define PCI_PPB_SECONDARY_BUS_NUM_OFFSET        0x19
#define PCI_PPB_SUBORDINATE_BUS_NUM_OFFSET      0x1A

#define _OSDEP_ISR_RETURN_T void
#define _OSDEP_ISR_RETURN_VAL(x) /* */

#define _OSDEP_BUS_BRIDGE_BIND_DONE(x) MOD_INC_USE_COUNT
#define _OSDEP_BUS_BRIDGE_UNBIND_DONE(X) MOD_DEC_USE_COUNT
#define _OSDEP_BUS_BRIDGE_BIND_REQ _osdep_bus_bridge_bind_req

typedef struct {
	struct pci_dev* bridge_child_pci_dev;
} _osdep_bridge_child_data_t;

typedef struct {
	int bridge_child_dummy;
} _osdep_bridge_rdata_t;

typedef struct _udi_intr_info {
	/*_OSDEP_ELEMENT_T link;*/
	void *context;
	int irq;
        _OSDEP_ISR_RETURN_T (*isr_func)(void *);	
} _udi_intr_info_t;

#endif /* __UDI_OSDEP_BRIDGE_H__ */
