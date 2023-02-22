
/*
 * File: mapper/linux-user/bridge/udi_bridge_linux.c
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

/* Mostly Common Code */

/* UDI includes */

#define	UDI_VERSION		0x101	/* 1.0 conformance */
#define	UDI_PHYSIO_VERSION	0x101

#include <udi_env.h>

int
posixuser_num_pci_devices()
{
	static int numdevs = -1;
	FILE *fd;

	if (numdevs == -1) {
		system("find /proc/bus/pci -type f | grep -v devices | wc -l > /tmp/numdevs");
		fd = fopen("/tmp/numdevs", "r");
		fscanf(fd, "%d", &numdevs);
		fclose(fd);
		unlink("/tmp/numdevs");
	}
	return numdevs;
}


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

#include <fcntl.h>

typedef union readdata {
	unsigned char c;
	unsigned short s;
	unsigned long l;
} readdata_t;

int
pci_cfg_open(char busnum,
	     char devnum,
	     char fnnum)
{
	int fd;
	char cfgname[255];

	sprintf(cfgname, "/proc/bus/pci/%02x/%02x.%01x",
		busnum, devnum, fnnum);
	fd = open(cfgname, O_RDONLY);
	return fd;
}

unsigned long
pci_read_cfg(int key,
	     int offset,
	     int size)
{
	readdata_t temp;
	unsigned long result = 0;

	pread(key, &temp, size, offset);
	switch (size) {
	case 1:
		result = temp.c;
		break;
	case 2:
#if _OSDEP_HOST_IS_BIG_ENDIAN
		temp.s = UDI_ENDIAN_SWAP_16(temp.s);
#endif
		result = temp.s;
		break;
	case 4:
#if _OSDEP_HOST_IS_BIG_ENDIAN
		temp.l = UDI_ENDIAN_SWAP_32(temp.l);
#endif
		result = temp.l;
		break;
	}
	return result;
}

void
pci_cfg_close(int key)
{
	close(key);
}

void
posixuser_enumerate_pci_devices(posix_pci_device_item_t * device_list)
{
	FILE *fd;
	char buf_[255], buf2_[255];
	char *buf, *buf2, *buf_end;
	udi_ubit32_t busnum, devnum, fnnum;
	unsigned long busdevfn;
	int numdevs;
	int key;

	numdevs = posixuser_num_pci_devices();

	buf = &buf_[0];
	buf2 = &buf2_[0];
	system("find /proc/bus/pci -type f | grep -v devices > /tmp/devlist");
	fd = fopen("/tmp/devlist", "r");
	while (numdevs--) {
		fscanf(fd, "%s", buf);
		strcpy(buf2, buf);
		buf += 14;		/* skip '/proc/bus/pci/' */
		buf[2] = 0;		/* overwrite the '/' */
		buf[5] = 0;		/* overwrite the '.' */
		busnum = udi_strtou32(&buf[0], &buf_end, 16);
		devnum = udi_strtou32(&buf[3], &buf_end, 16);
		fnnum = udi_strtou32(&buf[6], &buf_end, 16);
		busdevfn = (busnum << 8) | (devnum << 3) | (fnnum);
		/*
		 * fill in some fields. 
		 */
		device_list->device_present = 1;
		device_list->pci_unit_address = busdevfn;
		device_list->pci_slot = 0;

		key = pci_cfg_open(busnum, devnum, fnnum);

#define pcicfgread(offset, size) pci_read_cfg(key, offset, size);
		device_list->pci_vendor_id =
			pcicfgread(PCI_VENDOR_ID_OFFSET, 2);
		device_list->pci_device_id =
			pcicfgread(PCI_DEVICE_ID_OFFSET, 2);
		device_list->pci_subsystem_vendor_id =
			pcicfgread(PCI_SVEN_ID_OFFSET, 2);
		device_list->pci_subsystem_id =
			pcicfgread(PCI_SDEV_ID_OFFSET, 2);
		device_list->pci_revision_id =
			pcicfgread(PCI_REVISION_ID_OFFSET, 1);
		device_list->pci_base_class =
			pcicfgread(PCI_BASE_CLASS_OFFSET, 1);
		device_list->pci_sub_class =
			pcicfgread(PCI_SUB_CLASS_OFFSET, 1);
		device_list->pci_prog_if = pcicfgread(PCI_PROG_INTF_OFFSET, 1);

		pci_cfg_close(key);

#ifdef BRIDGE_DEBUG
		_OSDEP_PRINTF
			("Adding real PCI device %02x/%02x.%01x:%08lX vdid:%04X dvid:%04X\n",
			 busnum, devnum, fnnum, busdevfn,
			 device_list->pci_vendor_id,
			 device_list->pci_device_id);
#endif
		device_list += 1;	/* advance to the next item */
	}
	fclose(fd);
	unlink("/tmp/devlist");
}
