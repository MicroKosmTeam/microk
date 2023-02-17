
/*
 * src/env/uw/udi_readfile.c
 *
 * Environment routines which are used to implement readable files and
 * message files.
 *
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
#ifdef REALLY_READ_FROM_FILES
#include <udi_env.h>

#ifdef _KERNEL_HEADERS
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/conf.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <util/errno.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/types.h>
#else
#include <sys/cmn_err.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/vnode.h>
#endif

#define VNODE_STUFF 1

#if VNODE_STUFF

/*
 * This is here for the UW7 Kernel, only to duplicate the funtion
 * of the vn_assfail in the kernel, without DEBUG, since compiling
 * with DEBUG of the below causes problems.
 */
int
vn_assfail(vnode_t * vp,
	   const char *a,
	   const char *f,
	   int l)
{
	VN_LOG(vp, "vn_assfail");
	VN_UNLOCK(vp);
	cmn_err(CE_CONT, "VNODE FAILURE at 0x%lx\n", (long)vp);

	(void)conslog_set(CONSLOG_DIS);
	cmn_err(CE_PANIC, "assertion failed: %s, line:%d", a, f, l);
	/*
	 * NOTREACHED 
	 */
	return 1;
}

#pragma weak vx_getattr
extern int vx_getattr(struct vnode *,
		      struct vattr *,
		      int,
		      cred_t *);
char *
_udi_msgfile_search(const char *filename,
		    udi_ubit32_t msgnum,
		    const char *locale)
{
	int res, found, stage;
	char *buf, *ptr;
	udi_size_t buf_size;
	char *offset1, *offset2;
	vnode_t *vnodep;
	vattr_t vattrib;
	_vnodeops_v2_t *v_op;
	struct modobj *mp;
	int off;

	/*
	 * Temporary kludge, to insure this is not called with ipl > plbase 
	 */
	if (getpl() > PLBASE) {
		return (NULL);
	}

	/*
	 * Open up the message file, and read it into memory 
	 */
	if (lookupname
	    ((char *)filename, UIO_SYSSPACE, FOLLOW, NULLVPP, &vnodep) != 0) {
		_OSDEP_PRINTF("Unable to Open File %s\n", filename);
		return NULL;
	}

	vattrib.va_mask = AT_SIZE;
	if (vx_getattr == 0) {
		return "Function Not Supported";
	}

	res = vx_getattr(vnodep, &vattrib, 0, sys_cred);

	if (res != 0 || vattrib.va_size == 0) {
		VN_RELE(vnodep);
		return (NULL);
	}

	buf = (char *)_OSDEP_MEM_ALLOC(vattrib.va_size + 1, UDI_MEM_NOZERO,
				       UDI_WAITOK);

	if (vn_rdwr(UIO_READ, vnodep, buf, (off64_t) vattrib.va_size, 0,
		    UIO_SYSSPACE, 0, 0, CRED(), (int *)&buf_size) != 0) {
		_OSDEP_PRINTF("Unable to read message file: %s\n", filename);
		VN_RELE(vnodep);
		return (NULL);
	}
	VN_RELE(vnodep);
	buf_size = (udi_size_t)vattrib.va_size - buf_size;

	if (buf_size == 0)
		return (NULL);

	*(buf + buf_size) = '\0';

	/*
	 * Parse the buffer for a match 
	 */
	found = stage = 0;
	ptr = buf;
	while (ptr - buf < buf_size) {
		if (udi_strncmp(ptr, "locale ", 7) == 0) {
			/*
			 * Found a locale 
			 */
			offset1 = ptr + 7;
			offset2 = offset1;
			while (*offset2 != '\0' && *offset2 != '\n')
				offset2++;
			if (offset2 - offset1 == udi_strlen(locale) &&
			    udi_strncmp(locale, offset1,
					udi_strlen(locale)) == 0)
				stage = 1;
			else
				stage = 0;
		}
		if ((stage == 1 || udi_strcmp(locale, "C") == 0) &&
		    (udi_strncmp(ptr, "message ", 8) == 0 ||
		     udi_strncmp(ptr, "disaster_message ", 17)
		     == 0)) {
			/*
			 * Found a message in the appropriate locale 
			 */
			/*
			 * Point to a message number 
			 */
			if (*ptr == 'm')
				offset1 = ptr + 8;
			else
				offset1 = ptr + 17;
			/*
			 * Try to match the message number 
			 */
			if (msgnum == udi_strtou32(offset1, NULL, 10)) {
				/*
				 * Go to the start of the message 
				 */
				while (*offset1 != ' ')
					offset1++;
				offset1++;
				/*
				 * Find the end of the message 
				 */
				offset2 = offset1;
				while (*offset2 != '\n' && *offset2 != '\0')
					offset2++;
				found = 1;
				break;
			}
		}
		while (*ptr != '\n' && ptr < buf + buf_size)
			ptr++;
		if (*ptr == '\n')
			ptr++;
	}


	if (!found) {
		_OSDEP_MEM_FREE(buf);
		return (NULL);
	}

	/*
	 * Allocate some memory, and copy the string to return it 
	 */
	ptr = _OSDEP_MEM_ALLOC(offset2 - (offset1 - 1), UDI_MEM_NOZERO,
			       UDI_WAITOK);
	*(ptr + (offset2 - offset1)) = '\0';
	udi_memcpy(ptr, offset1, offset2 - offset1);

	/*
	 * Free the buffer memory 
	 */
	_OSDEP_MEM_FREE(buf);

	return (ptr);
}

void
_udi_readfile_getdata(const char *filename,
		      udi_size_t offset,
		      char *buf,
		      udi_size_t buf_len,
		      udi_size_t *act_len)
{
	vnode_t *vnodep;
	vattr_t vattrib;
	int res;

	/*
	 * Open up the readable file 
	 */
	if (vn_open((char *)filename, UIO_SYSSPACE, FREAD, 0, &vnodep, 0)) {
		_OSDEP_PRINTF("Unable to open readable file: %s\n", filename);
		return;
	}

	if (vn_rdwr(UIO_READ, vnodep, buf, buf_len, (off64_t) offset,
		    UIO_SYSSPACE, 0, 0, CRED(), &res) != 0) {
		_OSDEP_PRINTF("Unable to read readable file: %s\n", filename);
		VN_RELE(vnodep);
		*act_len = 0;
		return;
	}

	VN_RELE(vnodep);

	*act_len = buf_len - res;
	return;
}


#else  /* VNODE_STUFF */


char *
_udi_msgfile_search(const char *filename,
		    udi_ubit32_t msgnum,
		    const char *locale)
{
	char modname[20];
	char filepath[256];
	char *ptr, *ptr1, *ptr2;
	size_t filesz;
	char *buf;
	int found, stage;
	char *offset1, *offset2;

	/*
	 * yanking mdi_* out so disable these functions for now 
	 */
	return "Function Not Supported";
#if 0
	/*
	 * Figure out the modname/path for mdi_open_file 
	 */
	ptr1 = (char *)(filename + udi_strlen(filename) - 1);
	/*
	 * Get to /filename 
	 */
	while (*ptr1 != '/')
		ptr1--;
	ptr1--;
	/*
	 * Get to /msg/filename 
	 */
	while (*ptr1 != '/')
		ptr1--;
	ptr2 = ptr1;
	ptr1--;
	/*
	 * Get to /module/msg/filename 
	 */
	while (*ptr1 != '/')
		ptr1--;
	udi_strncpy(modname, ptr1 + 1, (ptr2 - ptr1) - 1);
	modname[(ptr2 - ptr1) - 1] = 0;
	ptr2++;
	if (udi_strncmp(filename, "/etc/conf/pack.d/", 17) == 0) {
		udi_strcpy(filepath, ptr2);
	} else {
		udi_strcpy(filepath, "../../../../");
		udi_strcat(filepath, filename);
	}

	/*
	 * open the file and read it in 
	 */

	/*
	 * Parse the buffer for a match 
	 */
	found = stage = 0;
	ptr = buf;
	while (ptr - buf < filesz) {
		if (udi_strncmp(ptr, "locale ", 7) == 0) {
			/*
			 * Found a locale 
			 */
			offset1 = ptr + 7;
			offset2 = offset1;
			while (*offset2 != '\0' && *offset2 != '\n')
				offset2++;
			if (offset2 - offset1 == udi_strlen(locale) &&
			    udi_strncmp(locale, offset1,
					udi_strlen(locale)) == 0)
				stage = 1;
			else
				stage = 0;
		}
		if ((stage == 1 || udi_strcmp(locale, "C") == 0) &&
		    (udi_strncmp(ptr, "message ", 8) == 0 ||
		     udi_strncmp(ptr, "disaster_message ", 17)
		     == 0)) {
			/*
			 * Found a message in the appropriate locale 
			 */
			/*
			 * Point to a message number 
			 */
			if (*ptr == 'm')
				offset1 = ptr + 8;
			else
				offset1 = ptr + 17;
			/*
			 * Try to match the message number 
			 */
			if (msgnum == udi_strtou32(offset1, NULL, 10)) {
				/*
				 * Go to the start of the message 
				 */
				while (*offset1 != ' ')
					offset1++;
				offset1++;
				/*
				 * Find the end of the message 
				 */
				offset2 = offset1;
				while (*offset2 != '\n' && *offset2 != '\0')
					offset2++;
				found = 1;
				break;
			}
		}
		while (*ptr != '\n' && ptr < buf + filesz)
			ptr++;
		if (*ptr == '\n')
			ptr++;
	}


	if (!found) {
		_OSDEP_MEM_FREE(buf);
		return (NULL);
	}

	/*
	 * Allocate some memory, and copy the string to return it 
	 */
	ptr = _OSDEP_MEM_ALLOC(offset2 - (offset1 - 1), UDI_MEM_NOZERO,
			       UDI_WAITOK);
	*(ptr + (offset2 - offset1)) = '\0';
	udi_memcpy(ptr, offset1, offset2 - offset1);

	/*
	 * Free the buffer memory 
	 */
	_OSDEP_MEM_FREE(buf);

	return (ptr);
#endif
}

void
_udi_readfile_getdata(const char *filename,
		      udi_size_t offset,
		      char *buf,
		      udi_size_t buf_len,
		      udi_size_t *act_len)
{
	char modname[20];
	char filepath[256];
	char *ptr, *ptr1, *ptr2;
	size_t filesz, nbytes;

	/*
	 * yanking mdi_* out so disable these functions for now 
	 */
	if (buf_len < 23) {
		*act_len = 23;
		return;
	}
	udi_strcpy(buf, "Function Not Supported");
	*act_len = 23;

#if 0
	/*
	 * Figure out the modname/path for mdi_open_file 
	 */
	ptr1 = (char *)(filename + udi_strlen(filename) - 1);
	/*
	 * Get to /filename 
	 */
	while (*ptr1 != '/')
		ptr1--;
	ptr1--;
	/*
	 * Get to /msg/filename 
	 */
	while (*ptr1 != '/')
		ptr1--;
	ptr2 = ptr1;
	ptr1--;
	/*
	 * Get to /module/msg/filename 
	 */
	while (*ptr1 != '/')
		ptr1--;
	udi_strncpy(modname, ptr1 + 1, (ptr2 - ptr1) - 1);
	modname[(ptr2 - ptr1) - 1] = 0;
	ptr2++;
	if (udi_strncmp(filename, "/etc/conf/pack.d/", 17) == 0) {
		udi_strcpy(filepath, ptr2);
	} else {
		udi_strcpy(filepath, "../../../../");
		udi_strcat(filepath, filename);
	}

	return;
#endif
}


#endif /* VNODE_STUFF */

#endif /* REALLY_READ_FROM_FILES */
