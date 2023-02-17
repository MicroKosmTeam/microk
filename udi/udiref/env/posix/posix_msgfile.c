
/*
 * File: env/posix/posix_sprops_elf.c
 *
 * UDI Posix Static Properties Elf-object-format interface
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

/*-----------------------------------------------------------------------
 *
 * If the current environment is a Posix build which uses ELF object file
 * formats...
 *
 * This file may be used to read the static driver properties from the
 * current source file (drv->drv_osinfo->driver_filename == posix_me).
 */

#include <udi_env.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <udi_std_sprops.h>
#include <posix.h>



_udi_std_mesgfile_t *
_OSDEP_GET_STD_MSGFILE(_udi_driver_t *drv,
		       char *msgfile_name,
		       udi_ubit32_t msgnum)
{
	int msgfile;
	struct stat msgstat;
	_udi_std_mesgfile_t *msg;
	char msgspec[512], *cp;

	/*
	 * Assume a Unix filename syntax (/root/subdir/subdir/filename) and take
	 * * our executable name, strip the executable part and find the message
	 * * file that was installed by udisetup.
	 */
	udi_strcpy(msgspec, posix_me);
	for (cp = &msgspec[udi_strlen(msgspec) - 1]; cp > msgspec; cp--)
		if (*cp == '/') {
			cp++;
			break;
		}			/* Unix assumption */
	*cp = 0;
	udi_strcat(msgspec, "msg/");
	udi_strcat(msgspec, _udi_osdep_sprops_get_shortname(drv));
	udi_strcat(msgspec, "/");
	udi_strcat(msgspec, msgfile_name);

	/*
	 * Now read in this message file 
	 */

	msgfile = open(msgspec, O_RDONLY);
	if (msgfile < 0) {
		_udi_env_log_write(_UDI_TREVENT_ENVERR,
				   UDI_LOG_WARNING, 0,
				   UDI_STAT_RESOURCE_UNAVAIL, 900,
				   drv->drv_name, msgfile_name, msgspec);
		/*
		 * Now continue so that we don't re-attempt access to this message 
		 * * file later but make sure we get no message definitions from this
		 * * file. 
		 */
		msgstat.st_size = 0;
	} else
		fstat(msgfile, &msgstat);

	msg = _OSDEP_MEM_ALLOC(msgstat.st_size + sizeof (*msg),
			       UDI_MEM_NOZERO, UDI_WAITOK);

	msg->nextmsgfile = NULL;
	udi_strncpy(msg->mesgfile_name, msgfile_name,
		    sizeof (msg->mesgfile_name) - 2);
	msg->mesgfile_name[sizeof (msg->mesgfile_name) - 1] = 0;
	msg->mesg_text = (char *)(msg + 1);	/*ptr arith */
	msg->mesg_len = msgstat.st_size;

	if (msgfile >= 0)
		if (read(msgfile, msg->mesg_text, msg->mesg_len) < 0) {
			_udi_env_log_write(_UDI_TREVENT_ENVERR,
					   UDI_LOG_ERROR, 0,
					   UDI_STAT_RESOURCE_UNAVAIL, 901,
					   drv->drv_name, msgfile_name,
					   msgspec);
			udi_memset(msg->mesg_text, 0, msg->mesg_len);
		}

	for (cp = msg->mesg_text + msg->mesg_len - 1; cp >= msg->mesg_text;
	     cp--)
		if (*cp == '\n')
			*cp = 0;

	close(msgfile);
	return msg;
}
