
/*
 * File: env/common/udi_std_sprops.h
 *
 * UDI Standard Static Properties handling.
 *
 * The std_sprops may be used by environments which obtain the static
 * properties as a set of strings and wish to parse and represent
 * those strings internally in a generic manner.
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
#include <udi_std_sprops.h>
#include <udi_sprops_ra.h>

#define _udi_isspace(C) ((C) == ' ' || (C) == '\t')

#define _sprops_get_word(to, from) { char *_tgt_str = (to);	\
    while (!_udi_isspace(*(from)) && *(from) != 0)		\
        *((_tgt_str)++) = *((from)++);				\
    *(_tgt_str) = 0;						\
    while (_udi_isspace(*(from))) (from)++;			\
}


#define propeq_start if (*cp == 0) { continue
#define propeq(S) } else if (udi_strncmp(cp,(S),udi_strlen(S)) == 0) { \
                                cp += udi_strlen(S);
#define propeq_end }

#define intprop(S) propeq("" #S " ") return udi_strtou32(cp, NULL, 0)


_udi_std_sprops_t *
udi_std_sprops_parse(char *propstrings,
		     int bytelength,
		     const char *drvname)
{
	char *cp, *sp = NULL;
	_udi_std_sprops_t *firstprops = NULL, *nextprops = NULL;
	int has_props = 0;

	for (cp = propstrings;
	     cp <= propstrings + bytelength; cp += udi_strlen(cp) + 1) {
		if ((udi_strncmp(cp, "properties_version ",
				 udi_strlen("properties_version ")) == 0)
		    || cp >= propstrings + bytelength) {
			/*
			 * Should probably check the actual version here... 
			 * This is a new properties_version or the very end 
			 * of them all.
			 */
			if (has_props) {
				firstprops =
					_OSDEP_MEM_ALLOC(sizeof
							 (_udi_std_sprops_t) +
							 (cp - sp), 0,
							 UDI_WAITOK);
				firstprops->nextprops = nextprops;
				firstprops->props_text = (void *)(firstprops + 1);	/*ptr arith */
				firstprops->props_len = (cp - sp);
				udi_memcpy(firstprops->props_text, sp,
					   firstprops->props_len);
				nextprops = firstprops;
			}
			sp = cp;
			has_props = 0;
		} else {
			if (*cp != 0) {
				if (drvname) {
					if (udi_strncmp(cp, "shortname ",
							udi_strlen
							("shortname ")) == 0)
						if (udi_strcmp
						    (cp +
						     udi_strlen("shortname "),
						     drvname) == 0) {
							has_props = 1;
						}
					/*
					 * else handle module and provides 
					 * specs as well?... 
					 */
				} else {
					has_props = 1;
				}
			}
		}
	}
	if (firstprops == NULL)
		DEBUG1(1005, 1, (drvname));
	UDIENV_CHECK(found_properties_for_specified_driver,
		     firstprops != NULL);

	return firstprops;
}


void
udi_std_sprops_free(_udi_std_sprops_t *propslist)
{
	_udi_std_sprops_t *nextprops;
	_udi_std_mesgfile_t *msgs, *nextmsgs;

	while (propslist && propslist != propslist->nextprops) {
		nextprops = propslist->nextprops;
		msgs = propslist->props_msgs;
		while (msgs) {
			nextmsgs = msgs->nextmsgfile;
			/*
			 * Perhaps the following should call an 
			 * _OSDEP_FREE_STD_MSGFILE rather than assuming 
			 * it's just flat memory, but that's a little too 
			 * much obfuscation for us at this point.
			 */

			_OSDEP_MEM_FREE(msgs);
			msgs = nextmsgs;
		}
		_OSDEP_MEM_FREE(propslist);
		propslist = nextprops;
	}
}



/*
 * Returns a count of the number of corresponding properties of this
 * type that exist for this driver.
 */

udi_ubit32_t
_udi_osdep_sprops_count(_udi_driver_t *drv,
			udi_ubit8_t type)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *prop = NULL, *cp;
	int propcnt = 0;

	/*
	 * Special case for udi MA "driver" which needs to count things
	 * before we have a real driver set up yet.
	 */
	if (sprops == NULL) {
		return 0;
	}

	switch (type) {
	case UDI_SP_META:
		prop = "meta ";
		break;
	case UDI_SP_CHILD_BINDOPS:
		prop = "child_bind_ops ";
		break;
	case UDI_SP_PARENT_BINDOPS:
		prop = "parent_bind_ops ";
		break;
	case UDI_SP_INTERN_BINDOPS:
		prop = "internal_bind_ops ";
		break;
	case UDI_SP_PROVIDE:
		prop = "provides ";
		break;
	case UDI_SP_REGION:
		prop = "region ";
		break;
	case UDI_SP_DEVICE:
		prop = "device ";
		break;
	case UDI_SP_READFILE:
		prop = "readable_file ";
		break;
	case UDI_SP_LOCALES:
		prop = "locale ";
		break;
	case UDI_SP_MESGFILE:
		prop = "message_file ";
		break;
	}

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq(prop);
		propcnt++;
		propeq_end;
	}

	return propcnt;
}


/*
 * Obtain the Static Properties Version number
 */
udi_ubit32_t
_udi_osdep_sprops_get_version(_udi_driver_t *drv)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		intprop(properties_version);
		propeq("properties_version ");
		return udi_strtou32(cp, NULL, 0);
		propeq_end;
	}
	return 0;
}


/*
 * Obtain the Static Properties Release number
 */
udi_ubit32_t
_udi_osdep_sprops_get_release(_udi_driver_t *drv)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		intprop(release);
		propeq_end;
	}
	return 0;
}

/*
 * Obtain the Staic Properties pio_serialization_domains value
 */
udi_sbit32_t
_udi_osdep_sprops_get_pio_ser_lim(_udi_driver_t *drv)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		intprop(pio_serialization_limit);
		propeq_end;
	}
	return -1;
}

/*
 * Obtain the Static Properties shortname
 */
char *
_udi_osdep_sprops_get_shortname(_udi_driver_t *drv)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("shortname ");
		return cp;
		propeq_end;
	}
	return 0;
}

const char *
_udi_get_shortname_from_sprops(_udi_std_sprops_t *sprops)
{
	_udi_driver_t drv;
	drv.drv_osinfo.sprops = sprops;
	return _udi_osdep_sprops_get_shortname(&drv);
}

const char *
_udi_osdep_sprops_get_descr(_udi_driver_t *drv)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;
	udi_ubit32_t i;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("name ");
		i = udi_strtou32(cp, &cp, 0);
		return _udi_osdep_sprops_ifind_message(drv, "C", i);
		propeq_end;
	}
	return 0;
}

/*
 * Obtain the Provides specification.  Index is which provides in the file
 * is to be accessed.
 */
char *
_OSDEP_SPROPS_GET_PROVIDES(_udi_driver_t *drv,
			   udi_ubit32_t index)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("provides ");
		if (index-- == 0)
			return cp;
		propeq_end;
	}
	return 0;
}


char *
_udi_osdep_sprops_get_prov_if(_udi_driver_t *drv,
			      udi_ubit32_t prov_inst)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;
	int prop_inst = 0;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;

		propeq("provides ");
		if (prop_inst++ == prov_inst)
			return cp;

		propeq_end;
	}

	return NULL;
}


udi_ubit32_t
_udi_osdep_sprops_get_prov_ver(_udi_driver_t *drv,
			       udi_ubit32_t prov_inst)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp, interface_name[512];
	int prop_inst = 0;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;

		propeq("provides ");
		if (prop_inst++ == prov_inst) {
			_sprops_get_word(interface_name, cp);
			return udi_strtou32(cp, &cp, 0);
		}

		propeq_end;
	}

	return 0xffffff;
}


/*
 * Given a locale, and the message number, return a pointer to the
 * message.  Note that the returned message is *still* in encoded
 * format (i.e. the UDI_ATTR_STRING Escape Sequences from Table 31-2
 * have not been applied); use _udi_attr_string_decode to perform the
 * translation.
 */

char *
_udi_osdep_sprops_ifind_message(_udi_driver_t *drv,
				char *locale,
				udi_ubit32_t num)
{
	char *cp, *ep, *default_msg = NULL;
	udi_boolean_t in_default_locale = TRUE;
	udi_boolean_t found_locale = FALSE;
	_udi_std_mesgfile_t *msgfile;
	int len;

	if (drv->drv_osinfo.sprops == NULL) {
		cp = _OSDEP_MEM_ALLOC(100, UDI_MEM_NOZERO, UDI_WAITOK);
		udi_snprintf(cp, 100, "udi_props not initialized yet.  "
			     "Cannot find message #%d.", num);
		return cp;
	}

	ep = drv->drv_osinfo.sprops->props_text +
		drv->drv_osinfo.sprops->props_len;
	if (udi_strcmp(locale, "C") == 0)
		found_locale = 1;

	for (cp = drv->drv_osinfo.sprops->props_text;
	     cp < ep; cp += udi_strlen(cp) + 1) {
		propeq_start;

		propeq("locale ");
		found_locale = (udi_strcmp(locale, cp) == 0);
		in_default_locale = (udi_strcmp(locale, "C") == 0);

		propeq("message ");
		if (udi_strtou32(cp, &cp, 0) == num) {
			if (found_locale) {
				cp += 1;
				goto found_msg;
			}
			if (in_default_locale)
				default_msg = cp + 1;
		}

		propeq("message_file ");
		for (msgfile = drv->drv_osinfo.sprops->props_msgs;
		     msgfile; msgfile = msgfile->nextmsgfile)
			if (udi_strcmp(msgfile->mesgfile_name, cp) == 0)
				break;
		if (msgfile == NULL) {
			msgfile = _OSDEP_GET_STD_MSGFILE(drv, cp, num);
			if (msgfile) {
				msgfile->nextmsgfile =
					drv->drv_osinfo.sprops->props_msgs;
				drv->drv_osinfo.sprops->props_msgs = msgfile;
			}
		}
		if (msgfile) {
			if (udi_strcmp(msgfile->mesgfile_name, cp) == 0) {
				char *cm, *em;
				udi_boolean_t correct_version = FALSE;

				cm = cp;
				em = msgfile->mesg_text + msgfile->mesg_len;
				for (cp = msgfile->mesg_text; cp < em;
				     cp += udi_strlen(cp) + 1) {
					propeq_start;

					propeq("properties_version ");
					if (udi_strtou32(cp, NULL, 0) !=
					    UDI_VERSION) {
						/*
						 * figure out how to parse some other version! 
						 */
						break;
					}
					correct_version = TRUE;

					propeq("locale ");
					if (correct_version) {
						found_locale =
							(udi_strcmp(locale, cp)
							 == 0);
						in_default_locale =
							(udi_strcmp
							 (locale, "C") == 0);
					}

					propeq("message ");
					if (correct_version) {
						if (udi_strtou32(cp, &cp, 0) ==
						    num) {
							if (found_locale) {
								cp += 1;
								goto found_msg;
							}
							if (in_default_locale)
								default_msg =
									cp + 1;
						}
					}

					propeq_end;
				}
				cp = cm;
			}
		}

		propeq_end;
	}

	if ((cp = default_msg) == NULL)
		return NULL;

      found_msg:
	len = udi_strlen(cp) + 1;
	ep = _OSDEP_MEM_ALLOC(len, UDI_MEM_NOZERO, UDI_WAITOK);
	udi_memcpy(ep, cp, len);
	return ep;
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
 *	UDI_SP_OP_OPSIDX	for ops_idx
 *	UDI_SP_OP_REGIONIDX	for region_idx
 *	UDI_SP_OP_PRIMOPSIDX	for primary_ops_idx
 *	UDI_SP_OP_SECOPSIDX	for secondary_ops_idx
 * (internal_bind_ops), passing an array index and a type.
 */

int
_udi_osdep_sprops_get_bindops(_udi_driver_t *drv,
			      udi_ubit32_t inst_index,
			      udi_ubit8_t bind_type,
			      udi_ubit8_t op_type)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *prop = NULL, *cp;
	int tgt_field = -1, field_val = -1;

	UDIENV_CHECK(bad_op_type_for_bind_type_1,
		     (bind_type == UDI_SP_CHILD_BINDOPS ?
		      (op_type != UDI_SP_OP_PRIMOPSIDX &&
		       op_type != UDI_SP_OP_SECOPSIDX) : 1));

	UDIENV_CHECK(bad_op_type_for_bind_type_2,
		     (bind_type == UDI_SP_PARENT_BINDOPS ?
		      (op_type != UDI_SP_OP_PRIMOPSIDX &&
		       op_type != UDI_SP_OP_SECOPSIDX) : 1));

	UDIENV_CHECK(bad_op_type_for_bind_type_3,
		     (bind_type == UDI_SP_INTERN_BINDOPS ?
		      op_type != UDI_SP_OP_OPSIDX : 1));

	switch (bind_type) {
	case UDI_SP_CHILD_BINDOPS:
		prop = "child_bind_ops ";
		break;
	case UDI_SP_PARENT_BINDOPS:
		prop = "parent_bind_ops ";
		break;
	case UDI_SP_INTERN_BINDOPS:
		prop = "internal_bind_ops ";
		break;
	default:
		UDIENV_CHECK(valid_bind_type, sprops == NULL);
		break;
	}

	switch (op_type) {
	case UDI_SP_OP_METAIDX:
		tgt_field = 1;
		break;
	case UDI_SP_OP_REGIONIDX:
		tgt_field = 2;
		break;
	case UDI_SP_OP_OPSIDX:
		tgt_field = 3;
		break;
	case UDI_SP_OP_PRIMOPSIDX:
		tgt_field = 3;
		break;
	case UDI_SP_OP_SECOPSIDX:
		tgt_field = 4;
		break;
	case UDI_SP_OP_CBIDX:
		tgt_field = (bind_type == UDI_SP_PARENT_BINDOPS) ? 4 : 5;
		break;
	default:
		UDIENV_CHECK(valid_bindop_type, sprops == NULL);
		break;
	}

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;

		propeq(prop);
		if (inst_index--)
			continue;	/* Skip to desired instance of prop */
		while (tgt_field--) {
			field_val = udi_strtou32(cp, &cp, 0);
			cp++;
		}
		break;

		propeq_end;
	}

	return field_val;
}



/*
 * Obtain the metalanguage interface name, based on a meta_idx
 */
udi_ubit32_t
_udi_osdep_sprops_get_meta_idx(_udi_driver_t *drv,
			       udi_ubit32_t meta_inst)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;
	int prop_inst = 0;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;

		propeq("meta ");
		if (prop_inst++ == meta_inst)
			return udi_strtou32(cp, &cp, 0);

		propeq_end;
	}

	return 0xffffff;
}


/*
 * Obtain the metalanguage interface name, based on a meta_idx
 */
char *
_udi_osdep_sprops_get_meta_if(_udi_driver_t *drv,
			      udi_ubit32_t meta_idx)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;

	if (meta_idx == 0)
		return "udi_mgmt";

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;

		propeq("meta ");
		if (udi_strtou32(cp, &cp, 0) != meta_idx)
			continue;
		cp++;
		break;

		propeq_end;
	}

	if (cp >= sprops->props_text + sprops->props_len)
		cp = "!! Unknown Metalanguage Interface !!";

	return cp;
}


/*
 * Get the <meta_idx> from the specified device line.
 */

udi_ubit32_t
_udi_osdep_sprops_get_dev_meta_idx(_udi_driver_t *drv,
				   udi_ubit32_t device_index)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;
	int dev_inst = 0;
	udi_ubit32_t msgnum;		/* discarded */

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("device ");
		if (dev_inst++ == device_index) {
			msgnum = udi_strtou32(cp, &cp, 0);
			return udi_strtou32(cp, &cp, 0);
		}
		propeq_end;
	}
	UDIENV_CHECK(Bad_Meta_idx_in_device_line, 0);
	return 0;
}


/*
 * Determine the number of attribute name/type/value tuples listed for a
 * device specification.
 *
 * The device_index is the relative index of the device specification
 * line within the limit determined by _OSDEP_SPROPS_COUNT(drv,
 * UDI_SP_DEVICE).
 */

udi_ubit32_t
_udi_osdep_sprops_get_dev_nattr(_udi_driver_t *drv,
				udi_ubit32_t device_index)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp, attrword[512];
	int dev_inst = 0;
	udi_ubit32_t msgnum, metaidx;	/* these will be discards */

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("device ");
		if (dev_inst++ == device_index) {
			msgnum = udi_strtou32(cp, &cp, 0);
			metaidx = udi_strtou32(cp, &cp, 0);
			/*
			 * Only skip to next field if we're not at end of line
			 */
			if (*cp != 0) {
				cp++;
				dev_inst = 1;	/* count the remaining words + 1 for last word */
				while (*cp != 0) {
					_sprops_get_word(attrword, cp);
					dev_inst++;
				}
				dev_inst /= 3;
			} else {
				dev_inst = 0;	/* No attributes */
			}
			return dev_inst;
		}
		propeq_end;
	}
	UDIENV_CHECK(Bad_number_of_attributes, 0);
	return 0;
}


/*
 * Get the name of a specific attribute for a "device" line in the udiprops.
 * The device_index is the index of which device line is to be used [within
 * the limit determined by _OSDEP_SPROPS_COUNT(drv, UDI_SP_DEVICE)] and the
 * attrib_index is the numeric index of which attribute is to be returned
 * [within the limit determined by _OSDEP_SPROPS_GET_DEV_NATTR].
 */

void
_udi_osdep_sprops_get_dev_attr_name(_udi_driver_t *drv,
				    udi_ubit32_t device_index,
				    udi_ubit32_t attrib_index,
				    char *namestr)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;
	int dev_inst = 0;
	udi_ubit32_t msgnum, metaidx;	/* these will be discards */

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("device ");
		if (dev_inst++ == device_index) {
			msgnum = udi_strtou32(cp, &cp, 0);
			metaidx = udi_strtou32(cp, &cp, 0);
			cp++;
			dev_inst = 0;	/* count the remaining words */
			while (*cp != 0) {
				_sprops_get_word(namestr, cp);
				if (dev_inst / 3 == attrib_index &&
				    dev_inst % 3 == 0)
					return;
				dev_inst++;
			}
			UDIENV_CHECK(bad_device_or_attribute_index, *cp != 0);
		}
		propeq_end;
	}

}

/*
 * Get the type of a specific attribute for a "device" line in the udiprops.
 * The device_index is the index of which device line is to be used [within
 * the limit determined by _OSDEP_SPROPS_COUNT(drv, UDI_SP_DEVICE)] and the
 * name is the name of the attribute on that device line (as returned by
 * _udi_osdep_sprops_get_dev_attr_name above).
 */

udi_instance_attr_type_t
_udi_osdep_sprops_get_dev_attr_type(_udi_driver_t *drv,
				    char *name,
				    udi_ubit32_t device_index)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp, attrword[512];
	int dev_inst = 0;
	udi_ubit32_t msgnum, metaidx;	/* these will be discards */

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("device ");
		if (dev_inst++ == device_index) {
			msgnum = udi_strtou32(cp, &cp, 0);
			metaidx = udi_strtou32(cp, &cp, 0);
			cp++;
			/*
			 * Find the named attribute 
			 */
			dev_inst = 0;	/* count the remaining words */
			while (*cp != 0) {
				_sprops_get_word(attrword, cp);
				if (dev_inst % 3 == 0 &&
				    udi_strcmp(name, attrword) == 0) {
					/*
					 * Found the attribute; determine and
					 * return its type.
					 */
					_sprops_get_word(attrword, cp);	/* get declared type */
					if (udi_strcmp(attrword, "string") ==
					    0)
						return UDI_ATTR_STRING;
					if (udi_strcmp(attrword, "ubit32") ==
					    0)
						return UDI_ATTR_UBIT32;
					if (udi_strcmp(attrword, "boolean") ==
					    0)
						return UDI_ATTR_BOOLEAN;
					return 0;	/* unhandled type */
				}
				dev_inst++;
			}
			UDIENV_CHECK(bad_device_or_attribute_index, *cp != 0);
		}
		propeq_end;
	}
	return 0;
}


/*
 * Get the value of a specific attribute for a "device" line in the udiprops.
 * The device_index is the index of which device line is to be used [within
 * the limit determined by _OSDEP_SPROPS_COUNT(drv, UDI_SP_DEVICE)] and the
 * name is the name of the attribute on that device line (as returned by
 * _udi_osdep_sprops_get_dev_attr_name above).
 */

udi_ubit32_t
_udi_osdep_sprops_get_dev_attr_value(_udi_driver_t *drv,
				     char *name,
				     udi_ubit32_t device_index,
				     udi_instance_attr_type_t attr_type,
				     void *valuep)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp, attrword[512];
	int dev_inst = 0;
	udi_ubit32_t msgnum, metaidx;	/* these will be discards */

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("device ");
		if (dev_inst++ == device_index) {
			msgnum = udi_strtou32(cp, &cp, 0);
			metaidx = udi_strtou32(cp, &cp, 0);
			cp++;
			/*
			 * Find the named attribute 
			 */
			dev_inst = 0;	/* count the remaining words */
			while (*cp != 0) {
				_sprops_get_word(attrword, cp);
				if (dev_inst % 3 == 0 &&
				    udi_strcmp(name, attrword) == 0) {
					/*
					 * Found the attribute; verify it's 
					 * the correct type and if so return 
					 * the attribute value. 
					 */
					_sprops_get_word(attrword, cp);	/* get declared type */
					switch (attr_type) {
					case UDI_ATTR_STRING:
						if (udi_strcmp
						    (attrword, "string")) {
							DEBUG1(1006, 5,
							       (drv->drv_name,
								device_index,
								name, "string",
								attrword));
							return (0);	/* invalid type */
						}
						/*
						 * Return the attribute value 
						 */
						_sprops_get_word(valuep, cp);
						return (udi_strlen(valuep));
					case UDI_ATTR_UBIT32:
						if (udi_strcmp
						    (attrword, "ubit32")) {
							DEBUG1(1006, 5,
							       (drv->drv_name,
								device_index,
								name, "ubit32",
								attrword));
							return (0);	/* invalid type */
						}
						_sprops_get_word(attrword, cp);
						*((udi_ubit32_t *)valuep) =
							udi_strtou32(attrword,
								     0, 0);
						return (4);
					case UDI_ATTR_BOOLEAN:
						if (udi_strcmp
						    (attrword, "boolean")) {
							DEBUG1(1006, 5,
							       (drv->drv_name,
								device_index,
								name,
								"boolean",
								attrword));
							return (0);	/* invalid type */
						}
						_sprops_get_word(attrword, cp);
						*((udi_boolean_t *)valuep) =
							(udi_strtou32(attrword,
								      0,
								      0) != 0);
						return (1);
					default:
						DEBUG1(1007, 5,
						       (drv->drv_name,
							device_index, name,
							attr_type, attrword));
						return (0);	/* unhandled type */
					}
				}
				dev_inst++;
			}
			UDIENV_CHECK(bad_device_or_attribute_index, *cp != 0);
		}
		propeq_end;
	}
	return (0);
}



/*
 * Get the region index value for the specified region instance.
 * The region_instance is the instance of which region line is to be used
 * [within the limit determined by _OSDEP_SPROPS_COUNT(drv, UDI_SP_REGION)].
 */

udi_ubit32_t
_udi_osdep_sprops_get_reg_idx(_udi_driver_t *drv,
			      udi_ubit32_t region_instance)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;
	udi_ubit32_t regnum, reginst = 0;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("region ");
		regnum = udi_strtou32(cp, &cp, 0);
		if (reginst++ != region_instance)
			continue;
		return regnum;
		propeq_end;		/* region */
	}
	UDIENV_CHECK(Bad_region_index, 0);
	return 0;
}


/*
 * Get the region attributes encoding for the specified region instance.
 * The region_instance is the instance of which region line is to be used
 * [within the limit determined by _OSDEP_SPROPS_COUNT(drv, UDI_SP_REGION)].
 */

udi_ubit32_t
_udi_osdep_sprops_get_reg_attr(_udi_driver_t *drv,
			       udi_ubit32_t region_instance)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;
	udi_ubit32_t regnum, reginst = 0, regattr = 0;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("region ");
		regnum = udi_strtou32(cp, &cp, 0);
		if (reginst++ != region_instance)
			continue;

		/*
		 * Scan the region attributes
		 */
		while (*cp != 0) {
			propeq_start;

#define _RAPROP(Spec, Value)  propeq(Spec); regattr |= _UDI_RA_##Value;

			propeq("type ");
			propeq_start;
			_RAPROP("normal", NORMAL);
			_RAPROP("fp", FP);
			_RAPROP("interrupt", INTERRUPT);
			propeq_end;	/* type */

			propeq("binding ");
			propeq_start;
			_RAPROP("static", STATIC);
			_RAPROP("dynamic", DYNAMIC);
			propeq_end;	/* binding */

			propeq("priority ");
			propeq_start;
			_RAPROP("lo", LO);
			_RAPROP("med", MED);
			_RAPROP("hi", HI);
			propeq_end;	/* priority */

			propeq("latency ");
			propeq_start;
			_RAPROP("powerfail_warning", POWER_WARN);
			_RAPROP("overrunable", OVERRUN);
			_RAPROP("retryable", RETRY);
			_RAPROP("non_overrunable", NON_OVERRUN);
			_RAPROP("non_critical", NON_CRITICAL);
			propeq_end;	/* latency */


			propeq("pio_probe ");
			propeq_start;
			propeq("no");
			regattr |= 0;
			_RAPROP("yes", PIO_PROBE);
			propeq_end;	/* pio_probe */

			propeq("overrun_time ");
			regattr += udi_strtou32(cp, &cp, 0);

			propeq_end;	/* named attribute */
			cp++;
		}
		propeq_end;		/* region */
	}

	/*
	 * Set defaults 
	 */
	if (!(regattr & _UDI_RA_TYPE_MASK))
		regattr |= _UDI_RA_NORMAL;
	if (!(regattr & _UDI_RA_BINDING_MASK))
		regattr |= _UDI_RA_STATIC;
	if (!(regattr & _UDI_RA_PRIORITY_MASK))
		regattr |= _UDI_RA_MED;
	if (!(regattr & _UDI_RA_LATENCY_MASK))
		regattr |= _UDI_RA_NON_OVERRUN;

	return regattr;
}



/*
 * Get the region attributes encoding for the specified region instance.
 * The region_instance is the instance of which region line is to be used
 * [within the limit determined by _OSDEP_SPROPS_COUNT(drv, UDI_SP_REGION)].
 */

udi_ubit32_t
_udi_osdep_sprops_get_reg_overrun(_udi_driver_t *drv,
				  udi_ubit32_t region_instance)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;
	udi_ubit32_t regnum, reginst = 0, regattr = 0;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("region ");
		regnum = udi_strtou32(cp, &cp, 0);
		if (reginst++ != region_instance)
			continue;

		/*
		 * Scan the region attributes
		 */
		while (*cp != 0) {
			propeq_start;

			propeq("type ");
			propeq_start;
			_RAPROP("normal", NORMAL);
			_RAPROP("fp", FP);
			_RAPROP("interrupt", INTERRUPT);
			propeq_end;	/* type */

			propeq("binding ");
			propeq_start;
			_RAPROP("static", STATIC);
			_RAPROP("dynamic", DYNAMIC);
			propeq_end;	/* binding */

			propeq("priority ");
			propeq_start;
			_RAPROP("lo", LO);
			_RAPROP("med", MED);
			_RAPROP("hi", HI);
			propeq_end;	/* priority */

			propeq("latency ");
			propeq_start;
			_RAPROP("powerfail_warning", POWER_WARN);
			_RAPROP("overrunable", OVERRUN);
			_RAPROP("retryable", RETRY);
			_RAPROP("non_overrunable", NON_OVERRUN);
			_RAPROP("non_critical", NON_CRITICAL);
			propeq_end;	/* latency */


			propeq("pio_probe ");
			propeq_start;
			propeq("no");
			regattr |= 0;
			_RAPROP("yes", PIO_PROBE);
			propeq_end;	/* pio_probe */

			propeq("overrun_time ");
			return udi_strtou32(cp, &cp, 0);

			propeq_end;	/* named attribute */
			cp++;
		}
		propeq_end;		/* region */
	}

	return 0;
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
	_OSDEP_PRINTF("_udi_osdep_sprops_get_rf_data UNIMPLEMENTED-TBD\n");
	return FALSE;
}

/*
 * Obtain the Readable File filename
 */
char *
_udi_osdep_sprops_get_readfile(_udi_driver_t *drv,
			       udi_ubit32_t inst)
{
	_udi_std_sprops_t *sprops = drv->drv_osinfo.sprops;
	char *cp;
	udi_ubit32_t rfinst = 0;

	for (cp = sprops->props_text;
	     cp < sprops->props_text + sprops->props_len;
	     cp += udi_strlen(cp) + 1) {
		propeq_start;
		propeq("readable_file ");
		if (rfinst++ == inst)
			return cp;
		propeq_end;
	}
	return 0;
}
