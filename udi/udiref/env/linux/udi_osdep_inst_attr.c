/*
 * File: env/linux/udi_osdep_inst_attr.c
 *
 * This code handles the init and deinit of dev_nodes, and
 * handles managing persistent static properties.
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

#include "udi_osdep_opaquedefs.h"
#include <udi_env.h>
#include <linux/pci.h>

/*
 * Add to osdep_dev_node_t...
 *   _OSDEP_MUTEX_T attr_storage_mutex;
 *   udi_queue_t attr_storage;
 */
/*
 * TBD: Make a /proc/udi/attributes/peristent/[dev_node_descriptor]
 * manager. That would be cool.
 * TBD: Make peristent attributes store on disk in a uniform way.
 */
/*
 * Note that peristent attributes do not worry about natural alignment.
 * This is because the definition of the storable data types and the
 * UDI_ATTR32_GET/SET accessor functions are all such that the data
 * is byte-aligned.
 * Note: _udi_instance_attr_get returns values in native endianness.
 */

typedef struct _udi_osdep_persistent_attr
{
	/* UDI queue header */
	udi_queue_t link;
	/* Modeled after the posix persistent properties. */
	udi_boolean_t is_valid;
	udi_size_t name_len;
	udi_ubit8_t data_type;
	udi_size_t data_len;
	udi_size_t container_len; /* data_len when first made */
	/* char name[name_len]; -- for portability, dont use gcc extensions. */
	/* char data[data_len]; -- for portability, dont use gcc extensions. */
} _udi_osdep_persistent_attr_t;

void _udi_dev_node_init(_udi_driver_t * driver, _udi_dev_node_t * udi_node)
{				/* New udi node */
    char bus_type[33];
    udi_instance_attr_type_t attr_type;
    udi_size_t length;
    _OSDEP_DEV_NODE_T *osinfo = &udi_node->node_osinfo;

#ifdef DEBUG
    _OSDEP_PRINTF("dev_node_init(drvr:%p, node:%p)\n", driver, udi_node);
#endif

    if (!osinfo->inited) {
	osinfo->inited = TRUE;
        _OSDEP_EVENT_INIT(&osinfo->enum_done_ev);
	/* This is the first time the node is being initialized. */
	UDI_QUEUE_INIT(&osinfo->attr_storage);
	_OSDEP_MUTEX_INIT(&osinfo->attr_storage_mutex,
					"node_osinfo.attr_storage_mutex");
    }
    if (driver == NULL) {
	return;
     }

    /* dev_node_init is being called again to map the driver to a device. */

    length = _udi_instance_attr_get(udi_node, "bus_type", bus_type,
				    sizeof(bus_type), &attr_type);

    if (length == 0 || (attr_type == UDI_ATTR_NONE)) {
	/* Don't map PCI device information if this isn't a device node. */
	return;
    }


    /*
     * Handle the PCI case
     */
    if (udi_strcmp(bus_type, "pci") == 0) {
	udi_ubit32_t unit_address = 0xFFFFFFFF,
			dev_num = 0xFFFFFFFF,
			vendor_num = 0xFFFFFFFF;

	struct pci_dev *devinfo = osinfo->node_devinfo;
#ifdef DEBUG
	_OSDEP_PRINTF("pci_dev %p found for dev_node %pn", devinfo, udi_node);
#endif	
	/*
	 * The below code is incomplete, and probably unneccessary.
	 * The osinfo->node_devinfo pointer should have been set by the
	 * bridge mapper during the bind of the driver to the bridge mapper.
	 * The below is left for the case where for some reason that does not
	 * happen. 
	 */
	if (!devinfo) {
		length = _udi_instance_attr_get(udi_node, "pci_unit_address",
					&unit_address,
					sizeof(udi_ubit32_t), &attr_type);

		if (length != 0) {
		    devinfo =
			pci_find_slot((int) ((unit_address >> 8) & 0xFF),
				      (int) (unit_address & 0xFF));
#ifdef DEBUG
		    _OSDEP_PRINTF("dev_node_init: matching by unit_address: %d:%d found devinfo %p\n",
				(int)((unit_address >> 8) & 0xFF),
				(int)(unit_address & 0xFF),
				devinfo);
#endif
		}
		if (!devinfo) {
		    /* All PCI devices must have this attribute */
		    _udi_instance_attr_get(udi_node, "pci_vendor_id",
					   &vendor_num, sizeof(udi_ubit32_t),
					   &attr_type);
	
		    /* All PCI devices must have this attribute */
		    _udi_instance_attr_get(udi_node, "pci_device_id", &dev_num,
					   sizeof(udi_ubit32_t), &attr_type);
			pci_find_device((unsigned int) vendor_num,
					(unsigned int) dev_num, NULL);
		}
/*		if (!devinfo)
		{
			_udi_instance_attr_get(udi_node, "pci_base_class",
				&class_num, sizeof(udi_ubit32_t), &attr_type);
			devinfo=pci_find_class((unsigned int)class_num, NULL);
		}*/
#ifdef DEBUG
		_OSDEP_PRINTF
		    ("dev_node_init: osinfo:%p devinfo:%p length:%ld unit_address=%X, dev_num=%X,"
        	     " vendor_num=%X\n",
		     osinfo, devinfo, length, unit_address, dev_num, vendor_num);
#endif

		if (devinfo) {
		    osinfo->node_devinfo = devinfo;
		} else {
#ifdef DEBUG
		    _OSDEP_PRINTF("dev_node_init: a matching pci device could not be found for driver %s\n",
			  driver->drv_name);
#endif
		}
	}
	return;

    } else if (udi_strcmp(bus_type, "isa") == 0) {
    } else if (udi_strcmp(bus_type, "eisa") == 0) {
    } else if (udi_strcmp(bus_type, "system") == 0) {
    }
#ifdef DEBUG
    _OSDEP_PRINTF("dev_node_init: Unknown bus_type: '%s'\n", bus_type);
#endif
}

void
_udi_osdep_enumerate_done(
	_udi_dev_node_t *udi_node){

	_OSDEP_DEV_NODE_T *osinfo = &udi_node->node_osinfo;

	_OSDEP_EVENT_SIGNAL(&osinfo->enum_done_ev);
}

void _udi_dev_node_deinit(_udi_dev_node_t * udi_node)
{
    _OSDEP_DEV_NODE_T *osinfo = &udi_node->node_osinfo;
    
    if (osinfo->inited == FALSE) {
	    _OSDEP_PRINTF("_udi_dev_node_deinit: called on uninited node\n");
	    return;
    }

    _OSDEP_EVENT_DEINIT(&osinfo->enum_done_ev);

    /* Tell attr_get/set to stop working */
    osinfo->node_devinfo = NULL;
    /*
     * If a mapped data structure belonging to the OS must be
     * notified of a detach, this is where the notification should happen.
     */

    /* Cleanup peristent attributes on this node. */
    _OSDEP_MUTEX_LOCK(&osinfo->attr_storage_mutex);
    while (!UDI_QUEUE_EMPTY(&osinfo->attr_storage)) {
	_OSDEP_MEM_FREE(UDI_BASE_STRUCT(UDI_DEQUEUE_HEAD(&osinfo->attr_storage),
					_udi_osdep_persistent_attr_t, link));
    }
    _OSDEP_MUTEX_UNLOCK(&osinfo->attr_storage_mutex);
    _OSDEP_MUTEX_DEINIT(&osinfo->attr_storage_mutex);
    udi_memset(&udi_node->node_osinfo, 0x00, sizeof(udi_node->node_osinfo));
}

void _udi_dump_attr_info(const char *name, udi_size_t length, udi_ubit8_t type);
void _udi_dump_attr_info(const char *name, udi_size_t length, udi_ubit8_t type)
{
	char str[256];
	char *sp;
	sp = str + sprintf(str, "Attr: %s Length: %d ", name, length);
	if (name[0] == '%') {
		sp += sprintf(str, "Class: Private, persistent ");
	}
	else if (name[0] == '@') {
		sp += sprintf(str, "Class: Parent-Visible, persistent ");
	}
	else if (name[0] == '^') {
		sp += sprintf(str, "Class: Sibling Group, non-persistent ");
	}
	else if (name[0] == '$') {
		sp += sprintf(str, "Class: Private, non-persistent ");
	}
	else {
		sp += sprintf(str, "Class: Enumeration, non-persistent ");
	}
	/* TBD: Search for a ':'. If found, it is a UDI_ATTR_FILE specifier. */
	switch (type) {
	case UDI_ATTR_NONE:
		sp += sprintf(str, "Type: none\n");
		break;
	case UDI_ATTR_UBIT32:
		sp += sprintf(str, "Type: uint32\n");
		break;
	case UDI_ATTR_FILE:
		sp += sprintf(str, "Type: file\n");
		break;
	case UDI_ATTR_ARRAY8:
		sp += sprintf(str, "Type: array\n");
		break;
	case UDI_ATTR_BOOLEAN:
		sp += sprintf(str, "Type: bool\n");
		break;
	default:
		sp += sprintf(str, "Type: unknown\n");
		break;
	}
#ifdef DEBUG
	_OSDEP_PRINTF(str);
#endif
}



udi_status_t
__osdep_instance_attr_set(_udi_dev_node_t *node,
			  const char *name,
			  const void *value,
			  udi_size_t length, udi_ubit8_t type)
{
	udi_queue_t *elem, *tmp;
	_udi_osdep_persistent_attr_t *thisattr;
	_OSDEP_DEV_NODE_T *osinfo = &node->node_osinfo;
	udi_size_t name_len;
	char *dataptr;

	name_len = strlen(name);
	printk("__osdep_inst_attr_set()\n");
#ifdef DEBUG
	_udi_dump_attr_info(name, length, type);
#endif
	if (osinfo->node_devinfo == NULL) {
		/* Can't set persistent attrs if there is no node_devinfo. */
		return UDI_STAT_RESOURCE_UNAVAIL;
	}

	/* Apply limits to known input parameters. */
	if (udi_strchr(name, ':')) {
		/*
		 * Fail because the user specified a name that assumes
		 * type == UDI_ATTR_FILE.
		 */
		return UDI_STAT_NOT_SUPPORTED;
	}
	switch (type) {
	case UDI_ATTR_FILE:
		/* Fail because:
		 * 1) The user asked to set a UDI_ATTR_FILE, but didn't
		 * specify a ':'.
		 * 2) File attributes are read-only. */
		return UDI_STAT_NOT_SUPPORTED;
	case UDI_ATTR_BOOLEAN:
		if (length != sizeof(udi_boolean_t)) {
			return UDI_STAT_NOT_SUPPORTED;
		}
		break;
	case UDI_ATTR_UBIT32:
		if (length != sizeof(udi_ubit32_t)) {
			return UDI_STAT_NOT_SUPPORTED;
		}
		break;
	case UDI_ATTR_NONE:
		if ((length != 0) && (value != NULL)) {
			return UDI_STAT_NOT_SUPPORTED;
		}
		break;
	default:
		if (length == 0) {
			return UDI_STAT_NOT_SUPPORTED;
		}
	}

	/*
	 * Try to modify an attribute that already exists, or 
	 * delete it if the type specified was UDI_ATTR_NONE.
	 */
	_OSDEP_MUTEX_LOCK(&osinfo->attr_storage_mutex);
	UDI_QUEUE_FOREACH(&osinfo->attr_storage, elem, tmp) {
		thisattr = UDI_BASE_STRUCT(elem,
					_udi_osdep_persistent_attr_t, link);
		if (thisattr->is_valid && thisattr->name_len == name_len) {
			/* Valid & name length's matched. */
			char *dataptr;
			dataptr = (char*)thisattr
					+ sizeof(_udi_osdep_persistent_attr_t);
			if (udi_memcmp(dataptr, name, name_len) == 0) {
				/* The name matched */
				if (type == UDI_ATTR_NONE) {
					/* delete this item */
					UDI_QUEUE_REMOVE(&thisattr->link);
					thisattr->is_valid = 0;
					_OSDEP_MEM_FREE(thisattr);
					_OSDEP_MUTEX_UNLOCK(&osinfo->
							attr_storage_mutex);
					return UDI_OK;
				}
				if (thisattr->container_len >= length) {
					/*
					 * Reuse this item, update data_len.
					 * This speeds up the case where UInt32
					 * types are used over and over.
					 */
					/*
					 * Notice the use of memmove. This
					 * solves memory alignment issues.
					 */
					dataptr += name_len;
					udi_memcpy(dataptr, value, length);
					thisattr->data_len = length;
					thisattr->data_type = type;
					_OSDEP_MUTEX_UNLOCK(&osinfo->
							attr_storage_mutex);
					return UDI_OK;
				}
				else {
					/* found it, we know we must realloc */
					break;
				}
			}
		}
	}
	_OSDEP_MUTEX_UNLOCK(&osinfo->attr_storage_mutex);

	/* Allocate a new item. */
	thisattr = (_udi_osdep_persistent_attr_t*)
		_OSDEP_MEM_ALLOC(sizeof(_udi_osdep_persistent_attr_t)
			+ name_len + length,
			UDI_MEM_NOZERO, UDI_NOWAIT);
	if (thisattr == NULL) {
		return UDI_STAT_RESOURCE_UNAVAIL;
	}
	thisattr->is_valid = 1;
	thisattr->name_len = name_len;
	thisattr->container_len = length;
	thisattr->data_len = length;
	thisattr->data_type = type;
	dataptr = (char*)thisattr + sizeof(_udi_osdep_persistent_attr_t);
	udi_memcpy(dataptr, name, name_len);
	dataptr += name_len;
	udi_memcpy(dataptr, value, length);

	_OSDEP_MUTEX_LOCK(&osinfo->attr_storage_mutex);
	/* Delete the item if it already exists. */
	UDI_QUEUE_FOREACH(&osinfo->attr_storage, elem, tmp) {
		char *dataptr;
		thisattr = UDI_BASE_STRUCT(elem,
					 _udi_osdep_persistent_attr_t, link);
		if (thisattr->is_valid && thisattr->name_len == name_len) {
			/* Valid & name length's matched. */
			dataptr = (char*)thisattr
					+ sizeof(_udi_osdep_persistent_attr_t);
			if (udi_memcmp(dataptr, name, name_len) == 0) {
				/* Name itself matched, this is the attr. */
				UDI_QUEUE_REMOVE(&thisattr->link);
				thisattr->is_valid = 0;
				_OSDEP_MEM_FREE(thisattr);
			}
		}
	}
	UDI_ENQUEUE_HEAD(&osinfo->attr_storage, &thisattr->link);
	_OSDEP_MUTEX_UNLOCK(&osinfo->attr_storage_mutex);

	return UDI_OK;
}

udi_size_t
__osdep_instance_attr_get(_udi_dev_node_t *node,
			  const char *name,
			  void *value,
			  udi_size_t length,
			  udi_ubit8_t *type)
{
	udi_queue_t *elem, *tmp;
	_udi_osdep_persistent_attr_t *thisattr;
	_OSDEP_DEV_NODE_T *osinfo = &node->node_osinfo;
	udi_size_t name_len, attr_size;
	printk("__osdep_inst_attr_get(%s)\n", name);

	/* Defaults on error */
	*type = UDI_ATTR_NONE;

	if (osinfo->node_devinfo == NULL) {
		/* The node has been deinited... no more attrs here. */
		return 0;
	}

#ifdef DEBUG
	_udi_dump_attr_info(name, length, -1); /* show as an unknown type */
#endif

	if (udi_strchr(name, ':') != 0) {
		/* The user specified a name that assumes a UDI_ATTR_FILE */
		if (*type != UDI_ATTR_FILE) {
			return 0;
		}
	}

	/* Default attr_size to 0 to match UDI_ATTR_NONE. */
	attr_size = 0;
	name_len = strlen(name);

	_OSDEP_MUTEX_LOCK(&osinfo->attr_storage_mutex);
	UDI_QUEUE_FOREACH(&osinfo->attr_storage, elem, tmp) {
		thisattr = UDI_BASE_STRUCT(elem, 
					_udi_osdep_persistent_attr_t, link);
		if (thisattr->is_valid && thisattr->name_len == name_len) {
			char *dataptr;
			dataptr = (char*)thisattr 
					+ sizeof(_udi_osdep_persistent_attr_t);
			if (udi_memcmp(dataptr, name, name_len) == 0) {
				udi_size_t copied;
				/* this is a match! */
				dataptr += name_len;
				*type = thisattr->data_type;
				copied = (length < thisattr->data_len ?
					     length : thisattr->data_len);
				udi_memcpy(value, dataptr, copied);
				/* Return stored size, not copied size. */
				attr_size = length;
				break;
			}
		}
	}
	_OSDEP_MUTEX_UNLOCK(&osinfo->attr_storage_mutex);
#if 1
	if (*type == UDI_ATTR_NONE) {
		/*
		 * Handle the case when the attribute was not found.
		 */
		udi_memset(value, 0x00, length);
	}
#endif
	return attr_size;
}

#if LATER
#include <linux/fs.h>
/*
 * This code somewhat works, it is here as an example of what's
 * desired in the future.
 */
void _udi_osdep_read_persistent_database()
{
	struct file *fd;
	int res;
	char *filename = "/tmp/udiinstattr.db";
	char buf[4096];
	int buf_len = 4096;


	/* Open up the readable file */
	fd = filp_open(filename, O_CREAT | O_RDONLY, /*octal*/0600);
	if (IS_ERR(fd)) {
		_OSDEP_PRINTF("Error %d when opening instattr file: '%s'\n",
			PTR_ERR(fd), filename);
		return attr_size;
	}

	/* FIX ME: walk the whole file, not just the first buf_len bytes... */
	res = generic_file_read(fd, buf, buf_len, /*offset*/0);
	filp_close(fd, NULL);
	if (IS_ERR(res)) {
		_OSDEP_PRINTF("Unable to read instattr file: '%s'  err: %d\n",
			filename, res);
		return attr_size;
	}
	buf_len = res;
}
#endif

