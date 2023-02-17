
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
#include <udi_osdep.h>

typedef struct {
	int meta_idx;
	char *if_name;
} _udi_sp_meta_t;

typedef struct {
	int meta_idx;
	int region_idx;
	int ops_idx;
	int cb_idx;
} _udi_sp_bindops_t;

typedef struct {
	int meta_idx;
	int region_idx;
	int primops_idx;
	int secops_idx;
	int cb_idx;
} _udi_sp_ibindops_t;

typedef struct {
	char *if_name;
	int ver;
} _udi_sp_provide_t;

typedef struct {
	int region_idx;
	unsigned int region_attr;
	unsigned int over_time;
} _udi_sp_region_t;

typedef struct {
	int meta_idx;
	int index;
	int num;
} _udi_sp_dev_t;

typedef struct {
	int meta_idx;
	unsigned int min_num;
	unsigned int max_num;
	int index;
	int num;
} _udi_sp_enum_t;

typedef struct {
	char *name;
	int type;
	union _udi_sp_attr_value {
		char *string;
		int integ;
	} value;
} _udi_sp_attr_t;

typedef struct {
	char *file;
	char *filename;
	unsigned int len;
	unsigned char *data;
} _udi_sp_readfile_t;

typedef struct {
	char *loc_str;
	int index;
	int num;
} _udi_sp_locale_t;

typedef struct {
	char *loc_str;
	char *filename;
} _udi_sp_msgfile_t;

typedef struct {
	int msgnum;
	char *text;
} _udi_sp_message_t;

#define _UDI_SP_SHORTNAME	0
#define _UDI_SP_VERSION		1
#define _UDI_SP_RELEASE		2
#define _UDI_SP_PIO_SER_LIM	3
#define _UDI_SP_METAS		4
#define _UDI_SP_CHILD_BINDOPS	5
#define _UDI_SP_PARENT_BINDOPS	6
#define _UDI_SP_INTERN_BINDOPS	7
#define _UDI_SP_PROVIDES	8
#define _UDI_SP_REGIONS		9
#define _UDI_SP_DEVICES		10
#define _UDI_SP_ENUMS		11
#define _UDI_SP_ATTRIBS		12
#define _UDI_SP_READ_FILES	13
#define _UDI_SP_LOCALE		14
#define _UDI_SP_READMSG_FILES	15
#define _UDI_SP_MESSAGES	16
#define _UDI_SP_MAX		17	/* This is the maximum number */

/*
 * This is the table which is used to obtain the correct index into
 * the _udi_sprops_idx_t array, based on the version number.
 *
 * Following the version and release numbers, are the indexes into the array to
 * find (in order):
 *	shortname
 *	version (must always be 1)
 *	release (must always be 2)
 *	pio_serialization_domains
 *	metas
 *	child_bindops
 *	parent_bindops
 *	intern_bindops
 *	provides
 *	regions
 *	devices
 *	enumerates
 *	attribs
 *	read_files
 *	locale
 *	message_files
 *	messages
 */
int _UDI_OSDEP_SPROPS_INDEX[2 * (_UDI_SP_MAX + 2)] = {
	UDI_VERSION, 1,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	0x0, 0,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

udi_ubit32_t _udi_osdep_sprops_get_version(_udi_driver_t *drv);
udi_ubit32_t _udi_osdep_sprops_get_release(_udi_driver_t *drv);
#ifdef REALLY_READ_FROM_FILES
char *_udi_msgfile_search(const char *file,
			  udi_ubit32_t msgnum,
			  const char *locale);
void _udi_readfile_getdata(const char *file,
			   udi_size_t offset,
			   char *buf,
			   udi_size_t buf_len,
			   udi_size_t *act_len);
#endif /* REALLY_READ_FROM_FILES */

int
_udi_osdep_sprops_get_index(_udi_driver_t *drv,
			int property)
{
	int invalid_udi_osdep_sprops_get_index = 0;
	int vers = _udi_osdep_sprops_get_version(drv);
	int rels = _udi_osdep_sprops_get_release(drv);
	int i;

	i = 0;
	while (_UDI_OSDEP_SPROPS_INDEX[i] != 0) {
		if (_UDI_OSDEP_SPROPS_INDEX[i] == vers &&
		    _UDI_OSDEP_SPROPS_INDEX[i + 1] == rels) {
			/*
			 * We have a match 
			 */
			_OSDEP_ASSERT(_UDI_OSDEP_SPROPS_INDEX[i + property + 2] >=
				      0);
			return (_UDI_OSDEP_SPROPS_INDEX[i + property + 2]);
		}
		i += (_UDI_SP_MAX + 2);
	}
	/*
	 * We should never get here 
	 */
	_OSDEP_ASSERT(invalid_udi_osdep_sprops_get_index);
	return (0);
}

void
_udi_osdep_sprops_init(_udi_driver_t *drv)
{
}


void
_udi_osdep_sprops_deinit(_udi_driver_t *drv)
{
	/*
	 * reverse any _OSDEP_SPROPS_INIT operations... 
	 */
}


/*
 * Sets the drv_osinfo sprops pointer
 */
void
_udi_osdep_sprops_set(_udi_driver_t *drv,
		  void *sprops_idx)
{
	drv->drv_osinfo.sprops = (_udi_sprops_idx_t *) sprops_idx;
}

/*
 * Obtain the Static Properties shortname
 */
char *
_udi_osdep_sprops_get_shortname(_udi_driver_t *drv)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_SHORTNAME);

	return ((char *)*(char **)(sprops->off));
}

/*
 * Obtain the Static Properties Version number
 */
udi_ubit32_t
_udi_osdep_sprops_get_version(_udi_driver_t *drv)
{
	/*
	 * This MUST always use (sprops+1) 
	 */
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops + _UDI_SP_VERSION;

	return (*(udi_ubit32_t *)(sprops->off));
}

/*
 * Obtain the Static Properties Release number
 */
udi_ubit32_t
_udi_osdep_sprops_get_release(_udi_driver_t *drv)
{
	/*
	 * This MUST always use (sprops+2) 
	 */
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops + _UDI_SP_RELEASE;

	return (*(udi_ubit32_t *)(sprops->off));
}

/*
 * Obtain the Static Properties pio_serailization_domains value
 */
udi_sbit32_t
_udi_osdep_sprops_get_pio_ser_lim(_udi_driver_t *drv)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
			_UDI_SP_PIO_SER_LIM;

	return (*(udi_sbit32_t *)(sprops->off));
}

/*
 * Given a pointer to the drv_osinfo in _udi_driver_t with a static
 * property type, return the number of that type of properties
 * available for parsing.
 */
udi_ubit32_t
_udi_osdep_sprops_count(_udi_driver_t *drv,
		    udi_ubit8_t type)
{
	int valid_sprops_type = 0;

	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops;

	switch (type) {
	case UDI_SP_META:
		return ((sprops + _udi_osdep_sprops_get_index(drv,
							  _UDI_SP_METAS))->
			num);
	case UDI_SP_CHILD_BINDOPS:
		return ((sprops + _udi_osdep_sprops_get_index(drv,
							  _UDI_SP_CHILD_BINDOPS))->
			num);
	case UDI_SP_PARENT_BINDOPS:
		return ((sprops + _udi_osdep_sprops_get_index(drv,
							  _UDI_SP_PARENT_BINDOPS))->
			num);
	case UDI_SP_INTERN_BINDOPS:
		return ((sprops + _udi_osdep_sprops_get_index(drv,
							  _UDI_SP_INTERN_BINDOPS))->
			num);
	case UDI_SP_PROVIDE:
		return ((sprops + _udi_osdep_sprops_get_index(drv,
							  _UDI_SP_PROVIDES))->
			num);
	case UDI_SP_REGION:
		return ((sprops + _udi_osdep_sprops_get_index(drv,
							  _UDI_SP_REGIONS))->
			num);
	case UDI_SP_DEVICE:
		return ((sprops + _udi_osdep_sprops_get_index(drv,
							  _UDI_SP_DEVICES))->
			num);
	case UDI_SP_READFILE:
		return ((sprops + _udi_osdep_sprops_get_index(drv,
							  _UDI_SP_READ_FILES))->
			num);
	case UDI_SP_LOCALES:
		return ((sprops + _udi_osdep_sprops_get_index(drv,
							  _UDI_SP_LOCALE))->
			num);
	case UDI_SP_READMSGFILE:
		return ((sprops + _udi_osdep_sprops_get_index(drv,
							  _UDI_SP_READMSG_FILES))->
			num);
	default:
		_OSDEP_ASSERT(valid_sprops_type);
		break;
	}

	/* We should never get here */
	return(0);
}

/*
 * Obtain the metalanguage index
 */
udi_ubit32_t
_udi_osdep_sprops_get_meta_idx(_udi_driver_t *drv,
			   udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_METAS);

#if 0
	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
#else
	_OSDEP_ASSERT(sprops->num != 0);
#endif
	return ((((_udi_sp_meta_t *) (sprops->off)) + index)->meta_idx);
}

/*
 * Obtain the metalanguage interface name, based on a meta_idx
 */
char *
_udi_osdep_sprops_get_meta_if(_udi_driver_t *drv,
			  udi_ubit32_t index)
{
	int n, j;
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_METAS);

	if (index == 0)
		return "udi_mgmt";

	n = _udi_osdep_sprops_count(drv, UDI_SP_META);
	/*
	 * Find a match 
	 */
	for (j = 0; j < n; j++) {
		if (((((_udi_sp_meta_t *) (sprops->off)) + j)->meta_idx) ==
		    index) {
			return ((((_udi_sp_meta_t *) (sprops->off)) + j)
				->if_name);
		}
	}
	return (0);
}

/*
 * Obtain the appropriate part of the child_bind_ops/parent_bind_ops/
 * internal_bind_ops, passing an index, bind_type, and op_type, where:
 * bind_type can be:
 *	UDI_SP_CHILD_BINDOPS
 *	UDI_SP_PARENT_BINDOPS
 *	UDI_SP_INTERN_BINDOPS
 * and op_type can be:
 *	UDI_SP_OP_METAIDX	for meta_idx
 *	UDI_SP_OP_REGIONIDX	for region_idx
 *	UDI_SP_OP_OPSIDX	for ops_idx
 *	UDI_SP_OP_PRIMOPSIDX	for primary_ops_idx
 *	UDI_SP_OP_SECOPSIDX	for secondary_ops_idx
 *	UDI_SP_OP_CBIDX		for cb_idx
 * (internal_bind_ops), passing an array index and a type.
 */
int
_udi_osdep_sprops_get_bindops(_udi_driver_t *drv,
			  udi_ubit32_t index,
			  udi_ubit8_t bind_type,
			  udi_ubit8_t op_type)
{
	int i, j;
	int valid_bind_type = 0;
	int valid_bindop_type = 0;

	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops;

	i = j = 0;

	switch (bind_type) {
	case UDI_SP_CHILD_BINDOPS:
		i = _udi_osdep_sprops_get_index(drv, _UDI_SP_CHILD_BINDOPS);
		break;
	case UDI_SP_PARENT_BINDOPS:
		i = _udi_osdep_sprops_get_index(drv, _UDI_SP_PARENT_BINDOPS);
		break;
	case UDI_SP_INTERN_BINDOPS:
		i = _udi_osdep_sprops_get_index(drv, _UDI_SP_INTERN_BINDOPS);
		break;
	default:
		_OSDEP_ASSERT(valid_bind_type);
		break;
	}

	if (bind_type != UDI_SP_INTERN_BINDOPS) {
		if (op_type != UDI_SP_OP_METAIDX &&
		    op_type != UDI_SP_OP_REGIONIDX &&
		    op_type != UDI_SP_OP_OPSIDX &&
		    op_type != UDI_SP_OP_CBIDX)
				_OSDEP_ASSERT(valid_bindop_type);
		if (op_type == UDI_SP_OP_CBIDX &&
		    bind_type == UDI_SP_CHILD_BINDOPS)
				_OSDEP_ASSERT(valid_bindop_type);
	} else {
		if (op_type != UDI_SP_OP_METAIDX &&
		    op_type != UDI_SP_OP_REGIONIDX &&
		    op_type != UDI_SP_OP_PRIMOPSIDX &&
		    op_type != UDI_SP_OP_SECOPSIDX &&
		    op_type != UDI_SP_OP_CBIDX)
				_OSDEP_ASSERT(valid_bindop_type);
	}

	switch (op_type) {
	case UDI_SP_OP_METAIDX:
		j = 0;
		break;
	case UDI_SP_OP_REGIONIDX:
		j = 1;
		break;
	case UDI_SP_OP_OPSIDX:
		j = 2;
		break;
	case UDI_SP_OP_PRIMOPSIDX:
		j = 2;
		break;
	case UDI_SP_OP_SECOPSIDX:
		j = 3;
		break;
	case UDI_SP_OP_CBIDX:
		if (bind_type == UDI_SP_PARENT_BINDOPS)
			j = 3;
		else
			j = 4;
		break;
	}
	_OSDEP_ASSERT((sprops + i)->num != 0 &&
		      (sprops + i)->num >= index + 1);
	if (bind_type != UDI_SP_INTERN_BINDOPS)
		return (*
			(((int
			   *)((_udi_sp_bindops_t *) ((sprops + i)->off) +
			      index)) + j));
	else
		return (*
			(((int
			   *)((_udi_sp_ibindops_t *) ((sprops + i)->off) +
			      index)) + j));
}

/*
 * Obtain the provide interface
 */
char *
_udi_osdep_sprops_get_prov_if(_udi_driver_t *drv,
			  udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_PROVIDES);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_provide_t *) (sprops->off)) + index)->if_name);
}

/*
 * Obtain the provide version
 */
udi_ubit32_t
_udi_osdep_sprops_get_prov_ver(_udi_driver_t *drv,
			   udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_PROVIDES);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_provide_t *) (sprops->off)) + index)->ver);
}

/*
 * Obtain the region index
 */
udi_ubit32_t
_udi_osdep_sprops_get_reg_idx(_udi_driver_t *drv,
			  udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_REGIONS);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_region_t *) (sprops->off)) + index)->region_idx);
}

/*
 * Obtain region attribute flag
 */
udi_ubit32_t
_udi_osdep_sprops_get_reg_attr(_udi_driver_t *drv,
			   udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_REGIONS);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_region_t *) (sprops->off)) + index)->region_attr);
}

/*
 * Obtain the region overrun_time
 */
udi_ubit32_t
_udi_osdep_sprops_get_reg_overrun(_udi_driver_t *drv,
			      udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_REGIONS);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_region_t *) (sprops->off)) + index)->over_time);
}

/*
 * Obtain the device parent metalanguage ops index
 */
udi_ubit32_t
_udi_osdep_sprops_get_dev_meta_idx(_udi_driver_t *drv,
			       udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_DEVICES);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_dev_t *) (sprops->off)) + index)->meta_idx);
}

/*
 * Obtain the number of device attributes
 */
udi_ubit32_t
_udi_osdep_sprops_get_dev_nattr(_udi_driver_t *drv,
			    udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_DEVICES);

#if 0
	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
#else
	_OSDEP_ASSERT(sprops->num != 0);
#endif
	return ((((_udi_sp_dev_t *) (sprops->off)) + index)->num);
}

/*
 * Obtain a designated device attribute
 */
void *
_udi_osdep_sprops_get_dev_attr(_udi_driver_t *drv,
			   udi_ubit32_t dev_index,
			   udi_ubit32_t att_index)
{
	_udi_sp_dev_t *sp_dev;
	_udi_sprops_idx_t *sprops1 = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_DEVICES);
	_udi_sprops_idx_t *sprops2 = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_ATTRIBS);

#if 0
	_OSDEP_ASSERT(sprops1->num != 0 && sprops1->num >= dev_index + 1);
#else
	_OSDEP_ASSERT(sprops1->num != 0);
#endif
	sp_dev = ((_udi_sp_dev_t *) (sprops1->off)) + dev_index;
#if 0
	_OSDEP_ASSERT(sp_dev->num != 0 && sp_dev->num >=
		      _udi_osdep_sprops_get_dev_nattr(drv, att_index));
#else
	_OSDEP_ASSERT(sp_dev->num != 0);
#endif
	return (((_udi_sp_attr_t *) (sprops2->off)) +
		(sp_dev->index + att_index));
}

/*
 * Obtain a designated device attribute name
 */
void
_udi_osdep_sprops_get_dev_attr_name(_udi_driver_t *drv,
				udi_ubit32_t dev_index,
				udi_ubit32_t att_index,
				char *namestr)
{
	_udi_sp_attr_t *ptr;

	ptr = _udi_osdep_sprops_get_dev_attr(drv, dev_index, att_index);
	udi_strcpy(namestr, ptr->name);
}


/*
 * Obtain the designated readable file
 */
char *
_udi_osdep_sprops_get_readfile(_udi_driver_t *drv,
			   udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_READ_FILES);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_readfile_t *) (sprops->off)) + index)->file);
}

/*
 * Obtain the full path/file of the readable file
 */
char *
_udi_osdep_sprops_get_freadfile(_udi_driver_t *drv,
			    udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_READ_FILES);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_readfile_t *) (sprops->off)) + index)->filename);
}

/*
 * Obtain the designated locale
 */
char *
_udi_sprops_get_locale(_udi_driver_t *drv,
		       udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_LOCALE);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_locale_t *) (sprops->off)) + index)->loc_str);
}

/*
 * Obtain the index for the start of the locale strings
 */
udi_ubit32_t
_udi_sprops_get_ilocale(_udi_driver_t *drv,
			udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_LOCALE);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_locale_t *) (sprops->off)) + index)->index);
}

/*
 * Obtain the number of the locale strings for the designated locale index
 */
udi_ubit32_t
_udi_sprops_get_nlocale(_udi_driver_t *drv,
			udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_LOCALE);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_locale_t *) (sprops->off)) + index)->num);
}

/*
 * Obtain a message index, by structure index, for the designated locale
 */
udi_ubit32_t
_udi_sprops_get_msg_idx(_udi_driver_t *drv,
			udi_ubit32_t loc_index,
			udi_ubit32_t str_index)
{
	udi_ubit32_t max_idx;
	udi_ubit32_t beg_idx;
	_udi_sprops_idx_t *sprops1 = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_LOCALE);
	_udi_sprops_idx_t *sprops2 = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_MESSAGES);

	_OSDEP_ASSERT(sprops1->num != 0 && sprops1->num >= loc_index + 1);
	max_idx = (((_udi_sp_locale_t *) (sprops1->off)) + loc_index)->num;
	_OSDEP_ASSERT(str_index < max_idx);
	beg_idx = (((_udi_sp_locale_t *) (sprops1->off)) + loc_index)->index;
	return ((((_udi_sp_message_t *)
		  (sprops2->off)) + str_index + beg_idx)->msgnum);
}

/*
 * Obtain a message string, by structure index, for the designated locale
 */
char *
_udi_sprops_get_msg(_udi_driver_t *drv,
		    udi_ubit32_t loc_index,
		    udi_ubit32_t str_index)
{
	udi_ubit32_t max_idx;
	udi_ubit32_t beg_idx;
	void *nstr, *ptr;
	udi_size_t len;

	_udi_sprops_idx_t *sprops1 = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_LOCALE);
	_udi_sprops_idx_t *sprops2 = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_MESSAGES);

	_OSDEP_ASSERT(sprops1->num != 0 && sprops1->num >= loc_index + 1);
	max_idx = (((_udi_sp_locale_t *) (sprops1->off)) + loc_index)->num;
	_OSDEP_ASSERT(str_index < max_idx);
	beg_idx = (((_udi_sp_locale_t *) (sprops1->off)) + loc_index)->index;
	ptr =
		(((_udi_sp_message_t *) (sprops2->off)) + str_index +
	       beg_idx)->text;
	len = udi_strlen(ptr) + 1;
	nstr = _OSDEP_MEM_ALLOC(len, UDI_MEM_NOZERO, UDI_WAITOK);
	udi_memcpy(nstr, ptr, len);
	return (nstr);
}

/*
 * Obtain the locale for the given message file index
 */
char *
_udi_sprops_get_mflocale(_udi_driver_t *drv,
			 udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_READMSG_FILES);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_msgfile_t *) (sprops->off)) + index)->loc_str);
}

/*
 * Obtain the filename for the given message file index
 */
char *
_udi_sprops_get_mffile(_udi_driver_t *drv,
		       udi_ubit32_t index)
{
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
		_udi_osdep_sprops_get_index(drv, _UDI_SP_READMSG_FILES);

	_OSDEP_ASSERT(sprops->num != 0 && sprops->num >= index + 1);
	return ((((_udi_sp_msgfile_t *) (sprops->off)) + index)->filename);
}

/*
 * Given a locale, and the message number, return a pointer to the message.
 */
char *
_udi_osdep_sprops_ifind_message(_udi_driver_t *drv,
			    char *locale,
			    udi_ubit32_t num)
{
	udi_ubit32_t nlocales, nstr, i, j, nmsgfiles;
	char *str;
	void *ptr;

	nlocales = _udi_osdep_sprops_count(drv, UDI_SP_LOCALES);
	/*
	 * For each available locale 
	 */
	for (i = 0; i < nlocales; i++) {
		if (udi_strcmp(locale, _udi_sprops_get_locale(drv, i))
		    == 0) {
			/*
			 * We matched the locale 
			 */
			/*
			 * How many strings for this locale? 
			 */
			nstr = _udi_sprops_get_nlocale(drv, i);
			for (j = 0; j < nstr; j++) {
				if (num == _udi_sprops_get_msg_idx(drv, i, j)) {
					return (_udi_sprops_get_msg
						(drv, i, j));
				}
			}
		}
	}
#ifdef REALLY_READ_FROM_FILES
	/*
	 * For each message file 
	 */
	nmsgfiles = _udi_osdep_sprops_count(drv, UDI_SP_READMSGFILE);
	for (i = 0; i < nmsgfiles; i++) {
		if (udi_strcmp(locale, _udi_sprops_get_mflocale(drv, i))
		    == 0) {
			/*
			 * We matched the locale 
			 */
			str =
				_udi_msgfile_search(_udi_sprops_get_mffile
						    (drv, i), num, locale);
			if (str != NULL)
				return (str);
		}
	}
#endif /* REALLY_READ_FROM_FILES */
	/*
	 * We did not find a string in the appropriate locale, check C 
	 */
	for (i = 0; i < nlocales; i++) {
		if (udi_strcmp("C", _udi_sprops_get_locale(drv, i))
		    == 0) {
			/*
			 * We matched the locale 
			 */
			/*
			 * How many strings for this locale? 
			 */
			nstr = _udi_sprops_get_nlocale(drv, i);
			for (j = 0; j < nstr; j++) {
				if (num == _udi_sprops_get_msg_idx(drv, i, j)) {
					return (_udi_sprops_get_msg
						(drv, i, j));
				}
			}
		}
	}
#ifdef REALLY_READ_FROM_FILES
	/*
	 * Lastly, check any C message files 
	 */
	for (i = 0; i < nmsgfiles; i++) {
		if (udi_strcmp("C", _udi_sprops_get_mflocale(drv, i))
		    == 0) {
			/*
			 * We matched the locale 
			 */
			str =
				_udi_msgfile_search(_udi_sprops_get_mffile
						    (drv, i), num, locale);
			if (str != NULL)
				return (str);
		}
	}
#endif /* REALLY_READ_FROM_FILES */
	/*
	 * We could not find the string associated with the number 
	 */
	ptr = _OSDEP_MEM_ALLOC(50, UDI_MEM_NOZERO, UDI_WAITOK);
	udi_snprintf(ptr, 50, "[Unknown message number %d.]", num);
	return (ptr);
}

/*
 * Given a locale, and a decimal number as a string, return the message.
 */
char *
_udi_sprops_afind_message(_udi_driver_t *drv,
			  char *locale,
			  char *msgnum)
{
	udi_ubit32_t num;

	num = udi_strtou32(msgnum, NULL, 0);
	return (_udi_osdep_sprops_ifind_message(drv, locale, num));
}

/*
 * Obtain a designated device attribute by name
 */
udi_ubit32_t
_udi_osdep_sprops_get_dev_attr_value(_udi_driver_t *drv,
				 char *name,
				 udi_ubit32_t dev_index,
				 udi_instance_attr_type_t attr_type,
				 void *valuep)
{
	udi_ubit32_t i, index;
	_udi_sp_attr_t *ptr;

	_OSDEP_ASSERT(dev_index < _udi_osdep_sprops_count(drv, UDI_SP_DEVICE));

	index = dev_index;
	*((udi_ubit32_t *)valuep) = 0;	/* zero enough to recognize null */

#if 0
	for (index = 0; index < _udi_osdep_sprops_count(drv, UDI_SP_DEVICE);
	     index++) {
#endif
		for (i = 0; i < _udi_osdep_sprops_get_dev_nattr(drv, index); i++) {
			ptr = _udi_osdep_sprops_get_dev_attr(drv, index, i);
			DEBUG4(4100, 2, (name, ptr->name));
			if ((strcmp(name, ptr->name) == 0)) {
				switch (attr_type) {
				case UDI_ATTR_STRING:
					DEBUG4(4101, 1, (attr_type));
					udi_strcpy(valuep, ptr->value.string);
					return (udi_strlen(valuep));
				case UDI_ATTR_UBIT32:
				case UDI_ATTR_BOOLEAN:
					DEBUG4(4102, 1, (attr_type));
					*((udi_ubit32_t *)valuep) =
						ptr->value.integ;
					if (attr_type == UDI_ATTR_UBIT32)
						return (4);
					else
						return (1);
				case UDI_ATTR_ARRAY8:
					DEBUG4(4103, 1, (attr_type));
					udi_memcpy(valuep,
					   ((udi_ubit8_t *)(ptr->value.string))+1,
					   (udi_size_t)(*(ptr->value.string)));
					return ((udi_ubit32_t)
							*(ptr->value.string));
				default:
					_OSDEP_PRINTF("type was %d\n",
						      attr_type);
					return (0);
				}
			}
		}
#if 0
	}
#endif
	return (0);
}

/*
 * Given the name of the readable file in instance attribute form,
 * along with the buffer location and length, get the appropriate data,
 * and set act_len.
 *
 * This should really only be invoked by udi_instance_attr_get.
 *
 * Returns TRUE if filename is in static props, otherwise FALSE.
 */
udi_boolean_t
_udi_osdep_sprops_get_rf_data(_udi_driver_t *drv,
			  const char *attr_name,
			  char *buf,
			  udi_size_t buf_len,
			  udi_size_t *act_len)
{
	char file[256];
	char *ptr;
	udi_size_t offset;
	udi_ubit32_t i, count;
	udi_boolean_t found;
	_udi_sprops_idx_t *sprops = drv->drv_osinfo.sprops +
			_udi_osdep_sprops_get_index(drv, _UDI_SP_READ_FILES);

	ptr = udi_strchr(attr_name, ':');
	if (ptr == NULL) {
		udi_strcpy(file, attr_name);
		offset = 0;
	} else {
		udi_strncpy(file, attr_name, (ptr - attr_name));
		file[ptr - attr_name] = '\0';
		ptr++;
		offset = udi_strtou32(ptr, NULL, 0);
	}
	count = _udi_osdep_sprops_count(drv, UDI_SP_READFILE);
	found = FALSE;
	for (i = 0; i < count; i++) {
		ptr = _udi_osdep_sprops_get_readfile(drv, i);
		if (udi_strcmp(ptr, file) == 0) {
#ifdef REALLY_READ_FROM_FILES
			ptr = _udi_osdep_sprops_get_freadfile(drv, i);
			_udi_readfile_getdata(ptr, offset, buf,
					      buf_len, act_len);
#else /* REALLY_READ_FROM_FILES */
			count = (((_udi_sp_readfile_t *)(sprops->off))+i)->len;
			ptr = (char *)(((_udi_sp_readfile_t *)
					(sprops->off))+i)->data;
			if (offset+buf_len <= count)
				i = offset+buf_len;
			else
				i = count;
			udi_memcpy(buf, ptr, i);
			*act_len = count - i;
#endif /* REALLY_READ_FROM_FILES */
			found = TRUE;
			break;
		}
	}

	if (found) {
		return (TRUE);
	} else {
		*act_len = 0;
		return (FALSE);
	}
}
