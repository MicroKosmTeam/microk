/*
 * File: solaris/osdep_pio.c
 *
 * PIO support routines for Solaris Kernel environment
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

#define	UDI_PCI_VERSION 0x101

#include "udi_env.h"
#include "udi_pci.h"

/*
 * This routine should _never_ be called - its here to fix up the external
 * reference in common/udi_env.h
 */
void _udi_get_pio_mapping(
	_udi_pio_handle_t *pio,
	_udi_dev_node_t	  *dev )
{
	udi_debug_printf("ERROR: _udi_get_pio_mapping called unexpectedly");
	udi_assert( 0 );
}

/*
 *  Map the given register set into the kernel access space. This allows the
 *  ddi_get/ddi_put and associated routines to be used with this pio handle.
 *  We have to know what the original driver supplied attributes were so that
 *  we can set the ddi_device_acc_attr fields correctly.
 */
void _udi_OS_get_pio_mapping( 
	_udi_pio_handle_t *pio,
	_udi_dev_node_t *dev,
	udi_ubit32_t	attributes)
{
	ddi_device_acc_attr_t	dev_attr;

	dev_attr.devacc_attr_version = DDI_DEVICE_ATTR_V0;

	/* Determine endian characteristics for the mapping */
	if (attributes & UDI_PIO_NEVERSWAP) {
		dev_attr.devacc_attr_endian_flags = DDI_NEVERSWAP_ACC;
	} else if (attributes & UDI_PIO_BIG_ENDIAN) {
		dev_attr.devacc_attr_endian_flags = DDI_STRUCTURE_BE_ACC;
	} else if (attributes & UDI_PIO_LITTLE_ENDIAN) {
		dev_attr.devacc_attr_endian_flags = DDI_STRUCTURE_LE_ACC;
	} else {
		/* Default to UDI_PIO_NEVERSWAP */
		dev_attr.devacc_attr_endian_flags = DDI_NEVERSWAP_ACC;
	}

	/* Determine data ordering for the mapping */
	if (attributes & UDI_PIO_STRICTORDER) {
		dev_attr.devacc_attr_dataorder = DDI_STRICTORDER_ACC;
	} else if (attributes & UDI_PIO_UNORDERED_OK) {
		dev_attr.devacc_attr_dataorder = DDI_UNORDERED_OK_ACC;
	} else if (attributes & UDI_PIO_MERGING_OK) {
		dev_attr.devacc_attr_dataorder = DDI_MERGING_OK_ACC;
	} else if (attributes & UDI_PIO_LOADCACHING_OK) {
		dev_attr.devacc_attr_dataorder = DDI_LOADCACHING_OK_ACC;
	} else if (attributes & UDI_PIO_STORECACHING_OK) {
		dev_attr.devacc_attr_dataorder = DDI_STORECACHING_OK_ACC;
	} else {
		/* Default to UDI_PIO_STRICTORDER */
		dev_attr.devacc_attr_dataorder = DDI_STRICTORDER_ACC;
	}
}
