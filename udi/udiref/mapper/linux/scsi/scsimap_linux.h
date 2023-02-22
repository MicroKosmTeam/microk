/*
 * File: mapper/linux/scsi/scsimap_linux.h
 *
 * Header file for the Linux SCSI mapper.
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

/* we don't need an OSDEP_SCSIMAP_RDATA_T  in Linux */
typedef unsigned char OSDEP_SCSIMAP_RDATA_T;

/* a pointer to the Scsi command structure */
typedef struct scsi_cmnd *OSDEP_SCSIMAP_REQ_T;

/* a pointer to the scsi host structure */
typedef struct Scsi_Host *OSDEP_SCSIMAP_HBA_T;

#define OSDEP_SCSIMAP_MAX_CMD_PER_LUN 	SCSIMAP_MAX_IO_CBS	/* (FIXME) pick a better value */
#define OSDEP_SCSIMAP_MAX_CMD_Q		SCSIMAP_MAX_IO_CBS	/* (FIXME) pick a better value */

/* the device registration daemon is an extra kernel thread */
#define osdep_scsimap_reg_daemon_init() do \
  if (kernel_thread((int (*)(void*))scsimap_reg_daemon_work, NULL, CLONE_FS) < 0) \
  { \
      _OSDEP_PRINTF("udi: failed to start the SCSI device registration daemon.\n"); \
      _OSDEP_ASSERT(0); \
  } while (0);

/*
 * We don't need these
 */
#define osdep_scsimap_region_init(r) /* */
#define osdep_scsimap_region_cleanup(r) /* */

