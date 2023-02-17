

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

#define _DDI 8

#include <udi_env.h>

#ifdef _KERNEL_HEADERS
#include <io/ddi.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <svc/errno.h>
#else
#include <sys/ddi.h>
#include <sys/cm_i386at.h>
#include <sys/confmgr.h>
#include <sys/resmgr.h>
#include <sys/errno.h>
#endif

#define DO_CM_GETBRDKEY _Compat_cm_getbrdkey
#define CM_GETVAL(p)	cm_getval(p)

#define RM_MAXPARAMLEN 16
struct rm_args {
	rm_key_t rm_key;
	char rm_param[RM_MAXPARAMLEN];
	void *rm_val;
	size_t rm_vallen;
	uint_t rm_n;
};

/* Warning: Use of kernel-internal (non-DDI) function here. */
extern int cm_find_match(struct rm_args *,
			 void *,
			 size_t);

extern int rm_nextparam(struct rm_args *);

/*
 * Search for the resmgr key for this node.
 * 
 * This is a bit nasty.  The logic is this
 *
 * 1st look for BRDBUSTYPE.
 * if (BRDBUSTYPE == PCI)
 *  then
 *     find the key based on pci_vendor_id,pci_device_id and pci_unit_address
 * elif (BRDBUSTYPE == SYSTEM || == ISA || == NONE)
 *  then
 *     find the key based MODNAME, skipping any records that have been seen before
 * fi
 * if no resmgr key is found, set it to RM_NULL_KEY
 */

void
_udi_dev_node_init(_udi_driver_t *driver,
		   _udi_dev_node_t *udi_node)
{					/* New udi node */
	_OSDEP_DEV_NODE_T *osinfo = &udi_node->node_osinfo;
	unsigned char buf[8];
	char bus_type[33];
	struct rm_args rma;
	udi_instance_attr_type_t attr_type;
	udi_size_t length;
	cm_args_t cma;
	int instnum;

	_OSDEP_EVENT_INIT(&osinfo->enum_done_ev);
	if (driver == NULL) {
		/*
		 * right after the node has been created 
		 */
		osinfo->rm_key = RM_NULL_KEY;
		return;
	}

	length = _udi_instance_attr_get(udi_node, "bus_type", bus_type,
					sizeof (bus_type), &attr_type);

	if (length == 0) {
		osinfo->rm_key = RM_NULL_KEY;
		return;
	}


	/*
	 * Handle the PCI case
	 */
	if (strcmp(bus_type, "pci") == 0) {
		udi_ubit32_t unit_address, func_num, dev_num, bus_num,
			ca_dev_config;


		_udi_instance_attr_get(udi_node, "pci_unit_address",
				       &unit_address, sizeof (udi_ubit32_t),
				       &attr_type);

		func_num = unit_address & 0x07;
		dev_num = (unit_address >> 3) & 0x1f;
		bus_num = (unit_address >> 8) & 0xff;
		ca_dev_config = (dev_num << 11) | (func_num << 8) | bus_num;

		strcpy(rma.rm_param, "CA_DEVCONFIG");
		rma.rm_val = buf;
		rma.rm_vallen = sizeof (cm_num_t);
		rma.rm_key = 0;
		if (cm_find_match(&rma, &ca_dev_config, sizeof (cm_num_t)) ==
		    0) {
			osinfo->rm_key = rma.rm_key;
		} else {
			osinfo->rm_key = RM_NULL_KEY;
		}
		goto got_key;

	} else if (strcmp(bus_type, "isa") == 0) {
		/*
		 * Put code in here to walk the resmgr, looking for matching
		 * modnames with BRBUSTYPE == ISA
		 */
	} else if (strcmp(bus_type, "eisa") == 0) {
		/*
		 * Put code in here to walk the resmgr, looking for matching
		 * modnames with BRBUSTYPE == EISA
		 */
	} else if (strcmp(bus_type, "system") == 0) {
		/*
		 * Put code in here to walk the resmgr, looking for matching
		 * modnames with BRBUSTYPE == SYSTEM
		 */
	}
	osinfo->rm_key = RM_NULL_KEY;
	return;

      got_key:

	cm_begin_trans(osinfo->rm_key, RM_READ);

	cma.cm_key = osinfo->rm_key;
	cma.cm_param = ".INSTNUM";
	cma.cm_val = &instnum;
	cma.cm_vallen = sizeof (instnum);
	cma.cm_n = 0;

	if (0 == cm_getval(&cma)) {
		udi_node->instance_number = instnum;
	}
	cm_end_trans(rma.rm_key);
}

void
_udi_dev_node_deinit(_udi_dev_node_t *udi_node)
{

	_OSDEP_DEV_NODE_T *osinfo = &udi_node->node_osinfo;

	_OSDEP_EVENT_DEINIT(&osinfo->enum_done_ev);
}

void
_osdep_udi_enumerate_done(_udi_dev_node_t *udi_node)
{

	_OSDEP_DEV_NODE_T *osinfo = &udi_node->node_osinfo;

	_OSDEP_EVENT_SIGNAL(&osinfo->enum_done_ev);
}

int
__osdep_udi_cm_trans(cm_args_t * cmarg,
		     int trans_type)
{

	int retval;

	cm_begin_trans(cmarg->cm_key, trans_type);
	if (trans_type == RM_READ)
		retval = cm_getval(cmarg);
	else
		retval = cm_addval(cmarg);
	cm_end_trans(cmarg->cm_key);
	return retval;
}

#define _UDI_NAME_LIST "UDI_NAME_LIST"
#define _UDI_MAX_NAME_LIST_LEN UDI_MAX_ATTR_NAMELEN + RM_MAXPARAMLEN + 2


#ifdef USE__OSDEP_INST_ATTR_GET_SHORTNAME
int
__osdep_inst_attr_get_shortname(_udi_dev_node_t *node,
				const char *long_name,
				char *short_name,
				udi_ubit8_t *attr_type,
				int *num_names)
{

	cm_args_t cmarg;
	int retval, i;
	char *value;
	char buf[UDI_MAX_ATTR_NAMELEN];
	udi_size_t long_name_len;


	udi_memset(short_name, 0, RM_MAXPARAMLEN);
	*attr_type = UDI_ATTR_NONE;
	value = kmem_zalloc(_UDI_MAX_NAME_LIST_LEN, KM_SLEEP);
	cmarg.cm_key = node->node_osinfo.rm_key;
	cmarg.cm_val = value;
	cmarg.cm_vallen = _UDI_MAX_NAME_LIST_LEN;
	cmarg.cm_param = _UDI_NAME_LIST;
	for (i = 0;; i++) {
		cmarg.cm_n = i;
		retval = __osdep_udi_cm_trans(&cmarg, RM_READ);
		if (retval == ENOENT) {
			*num_names = i;
			return 0;
		} else if (retval != 0)
			return retval;
		long_name_len =
			udi_strchr(cmarg.cm_val,
				   '|') - (char *)cmarg.cm_val - 1;
		udi_strncpy(buf, cmarg.cm_val, long_name_len);
		buf[long_name_len] = 0;
		if (strcmp(long_name, buf) == 0) {
			if (strlen(long_name) > RM_MAXPARAMLEN - 3)
				strncpy(short_name,
					udi_strchr(cmarg.cm_val, '|') + 1,
					RM_MAXPARAMLEN);
			else
				strncpy(short_name,
					udi_strchr(cmarg.cm_val, '|') + 1,
					strlen(long_name));

			*attr_type = (udi_ubit8_t)udi_strtou32((char *)cmarg.
							       cm_val +
							       cmarg.
							       cm_vallen - 1,
							       NULL, 16);
			kmem_free(value, _UDI_MAX_NAME_LIST_LEN);
			break;
		}
	}
	return 0;
}
#endif /* USE__OSDEP_INST_ATTR_GET_SHORTNAME */

udi_status_t
__osdep_instance_attr_set(_udi_dev_node_t *node,
			  const char *name,
			  const void *value,
			  udi_size_t length,
			  udi_ubit8_t type)
{

	cm_args_t cmarg;
	char short_name[RM_MAXPARAMLEN];
	char *name_list;
	int retval, num_name;
	udi_ubit8_t dummy_type;
	char lower_name[UDI_MAX_ATTR_NAMELEN];
	udi_size_t i, name_size;

	if ((cmarg.cm_key = node->node_osinfo.rm_key) == RM_NULL_KEY)
		return UDI_STAT_RESOURCE_UNAVAIL;
	num_name = 0;
	for (i = 0; name[i] != 0 && i < UDI_MAX_ATTR_NAMELEN; i++)
		lower_name[i] = (char)_udi_tolower(name[i]);
	lower_name[i] = 0;
#ifdef USE__OSDEP_INST_ATTR_GET_SHORTNAME
	if (__osdep_inst_attr_get_shortname(node, lower_name, short_name,
					    &dummy_type, &num_name) != 0)
		return UDI_STAT_RESOURCE_UNAVAIL;
#endif /* USE__OSDEP_INST_ATTR_GET_SHORTNAME */
	if (strlen(short_name) == 0) {
		/*
		 * generate a new shortname 
		 */
		if (strlen(lower_name) > RM_MAXPARAMLEN - 1) {
			strncpy(short_name, lower_name, RM_MAXPARAMLEN - 3);
			udi_snprintf(short_name + RM_MAXPARAMLEN - 3, 3,
				     "%02x", num_name);
		} else {
			strncpy(short_name, lower_name, RM_MAXPARAMLEN - 1);
		}
		/*
		 * save longname, shortname and attr type in UDI_NAME_LIST 
		 */
		name_size =
			udi_strlen(lower_name) + udi_strlen(short_name) + 3;
		name_list = kmem_zalloc(name_size, KM_SLEEP);
		udi_snprintf(name_list, name_size, "%s|%s|%x", lower_name,
			     short_name, type);
		cmarg.cm_param = _UDI_NAME_LIST;
		cmarg.cm_val = (void *)name_list;
		cmarg.cm_vallen = strlen(name_list) + 1;
		if ((retval = __osdep_udi_cm_trans(&cmarg, RM_RDWR)) != 0)
			return UDI_STAT_RESOURCE_UNAVAIL;
	}
	cmarg.cm_param = short_name;
	cmarg.cm_val = (void *)value;
	cmarg.cm_vallen = length;
	retval = __osdep_udi_cm_trans(&cmarg, RM_RDWR);
	return retval ? UDI_STAT_RESOURCE_UNAVAIL : UDI_OK;

}

/* 
 * Since we are not supporting long attr names, we limit them to
 * 14 characters, and then we steal the first byte for the attr 
 * type, so we limit ourselves to 13 bytes while comparing attr names.
 */
#define BOGUS_NAME_LIMIT 13

udi_size_t
__osdep_instance_attr_get(_udi_dev_node_t *node,
			  const char *name,
			  void *value,
			  udi_size_t length,
			  udi_ubit8_t *type)
{
	struct rm_args rmarg;
	cm_args_t cmarg;
	char short_name[RM_MAXPARAMLEN];
	char lower_name[UDI_MAX_ATTR_NAMELEN];
	char rm_lower_name[RM_MAXPARAMLEN];
	char cm_value[RM_MAXPARAMLEN];
	int dummy_int;
	udi_size_t i;
	int rmretval;

	*type = UDI_ATTR_NONE;
	cmarg.cm_key = node->node_osinfo.rm_key;
	if (cmarg.cm_key == RM_NULL_KEY)
		return 0;
	for (i = 0; name[i] != 0 && i < UDI_MAX_ATTR_NAMELEN; i++)
		lower_name[i] = (char)_udi_tolower(name[i]);
	lower_name[i] = 0;
#ifdef USE__OSDEP_INST_ATTR_GET_SHORTNAME
	if (__osdep_inst_attr_get_shortname(node, lower_name, short_name,
					    type, &dummy_int) != 0 ||
	    strlen(short_name) == 0)
		return 0;
#endif /* USE__OSDEP_INST_ATTR_GET_SHORTNAME */
	/*
	 * find the param name in resmgr database 
	 */
	rmarg.rm_key = cmarg.cm_key;
	for (rmretval = 0, rmarg.rm_n = 1; rmretval == 0; rmarg.rm_n++) {
		cm_begin_trans(rmarg.rm_key, RM_READ);
		rmretval = rm_nextparam(&rmarg);
		cm_end_trans(rmarg.rm_key);
		if (rmretval != 0)	/* end of param list, no match, bail out */
			return 0;
		/*
		 * convert the param name to lower case before the comparison 
		 */
		for (i = 0; rmarg.rm_param[i] != 0 && i < RM_MAXPARAMLEN; i++)
			rm_lower_name[i] =
				(char)_udi_tolower(rmarg.rm_param[i]);
		rm_lower_name[i] = 0;
		/*
		 * skip the 'attr type' byte in rm_lower_name 
		 */
		if (udi_strncmp
		    ((*lower_name == '%') ? lower_name + 1 : lower_name,
		     rm_lower_name + 1, BOGUS_NAME_LIMIT) == 0)
			break;
	}
	cmarg.cm_vallen = length;
	cmarg.cm_val = cm_value;
	cmarg.cm_n = 0;
	cmarg.cm_param = rmarg.rm_param;

	if (__osdep_udi_cm_trans(&cmarg, RM_READ) != 0)
		return 0;

	/*
	 * pull the first byte out for attr type (added in bcfg file) 
	 */
	switch (*cmarg.cm_param) {
	case 'U':
	case 'u':
		{
			udi_ubit32_t *tmpval = (udi_ubit32_t *)value;

			*tmpval = udi_strtou32((char *)(cm_value), NULL, 10);
		}
		*type = UDI_ATTR_UBIT32;
		break;
	case 'S':
	case 's':
		{
			char *tmpval = (char *)value;

			strcpy(tmpval, cm_value);
		}
		*type = UDI_ATTR_STRING;
		break;
	case 'B':
	case 'b':
		{
			udi_ubit8_t *tmpval = (udi_ubit8_t *)value;

			if ((cm_value[0] == 'F') || (cm_value[0] == 'f'))
				*tmpval = FALSE;
			else if ((cm_value[0] == 'T') || (cm_value[0] == 't'))
				*tmpval = TRUE;
		}
		*type = UDI_ATTR_BOOLEAN;
		break;
#if 0
		/*
		 * Not implemented yet 
		 */
	case 'A':
	case 'a':
		*type = UDI_ATTR_ARRAY8;
		break;
#endif
	default:
		*type = UDI_ATTR_NONE;
	}
	return (udi_size_t)cmarg.cm_vallen;
}

STATIC udi_boolean_t
_udi_match_rmkey(void *arg1,
		 void *arg2)
{

	_OSDEP_DEV_NODE_T *uw_node = (_OSDEP_DEV_NODE_T *) arg1;
	rm_key_t key = *(rm_key_t *) arg2;

	return (uw_node->rm_key == key);
}

_udi_dev_node_t *
_udi_find_node_by_rmkey(rm_key_t key)
{

	return _udi_MA_find_node_by_osinfo(_udi_match_rmkey, (void *)&key);
}
