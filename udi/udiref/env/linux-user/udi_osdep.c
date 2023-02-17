/*
 * File: env/linux-user/udi_osdep.c
 *
 * This is the linuxstub osdep code for the posix on linux environment
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

#if defined(__powerpc__) || defined(__sparc)
extern int   ioperm(long port, int len, char priv_ok);
int   ioperm(long port, int len, char priv_ok)
{(void)port;(void)len;(void)priv_ok;return 0;}
#else
/* io defn's for userland work fine on x86 */
#include <sys/io.h>
#endif

/* #include <../../mapper/common/bridge/udi_bridgeP.h> */
#include <stdarg.h>

#ifndef __va_arg_type_violation
void __va_arg_type_violation(void)
{
	int *ptr = (int*)0;
	fprintf(stderr, "\n\nlinux-user/udi_osdep.c.__va_arg_type_violation() called\n\n");
noreturn:
	*ptr = 0; /* STOP! */
	goto noreturn;
}
#endif

/*
 * On Linux, ioperm works on a per-thread basis. So, we need to
 * change the ioperm such that it happens in the same thread that's
 * about to execute the trans-list.
 */
void
_udi_osdep_pio_map(_udi_pio_handle_t *pio)
{
	switch (pio->address_type) {
	case _UDI_PIO_IO_SPACE:
		if (ioperm((unsigned long)pio->pio_osinfo._u.io.addr,
			pio->length, TRUE)) {
			fprintf(stderr, 
			"Unable to set ioperm for 0x%lx.%s\n", pio->offset,
			strerror(errno)); 
		exit(1); 
		}
		/* Other cases need no special handling. */
	default:
		break;
	}
}


int
linux_posix_dev_node_init(_udi_driver_t *driver,	/* New UDI node */
		    _udi_dev_node_t *udi_node) /* Parent UDI node */
{
	char bus_type[33];
	udi_instance_attr_type_t attr_type;
	udi_size_t length;
	_OSDEP_DEV_NODE_T *osinfo = &udi_node->node_osinfo;

	posix_dev_node_init(driver, udi_node);

	if (driver == NULL) {
		return 0;
	}

	/* Do some PCI mapping. */
	length = _udi_instance_attr_get(udi_node, "bus_type", bus_type,
				    sizeof(bus_type), &attr_type);

	if (length == 0 || (attr_type == UDI_ATTR_NONE)) {
		/* Don't map PCI device information if this isn't a device node. */
#ifdef DEVNODE_DEBUG
		_OSDEP_PRINTF("%p isn't a bus device node.\n", udi_node);
#endif
		return 0;
	}
#ifdef DEVNODE_DEBUG
	_OSDEP_PRINTF("%p is a bus device node.\n", udi_node);
#endif

	/* Handle the PCI case */
	if (udi_strcmp(bus_type, "pci") == 0) {
		udi_ubit32_t unit_address = 0xFFFFFFFF;

		/* All PCI devices must have this attribute */
		length = _udi_instance_attr_get(udi_node, "pci_unit_address",
					&unit_address,
					sizeof(udi_ubit32_t), &attr_type);

		if (length == 0 || (attr_type == UDI_ATTR_NONE)) {
			osinfo->posix_dev_node.node_pci_unit_address = 0xFFFFFFFF;
#ifdef DEVNODE_DEBUG
			_OSDEP_PRINTF("%p isn't a PCI device node.\n", udi_node);
#endif
			return 0;
		} else {
			osinfo->posix_dev_node.node_pci_unit_address = unit_address;
#ifdef DEVNODE_DEBUG
			_OSDEP_PRINTF("%p is a PCI device node.\n", udi_node);
#endif
			return 0;
		}
	} else if (udi_strcmp(bus_type, "system") == 0) {
	}
	return 0;
}

int
linux_posix_dev_node_deinit(_udi_dev_node_t *udi_node) /* Parent UDI node */
{
	posix_dev_node_deinit(udi_node);
	return 0;
}

