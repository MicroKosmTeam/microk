/*
 * File: giomap_linux.h
 *
 * The all inclusive Linux GIO mapper include file.
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

#if 1
#define paddr_t void*
#define daddr_t char*
#define uio_t void
#define _B_ADDR         union { \
                           caddr_t b_addr;      /* address of buffer data */ \
                           void *__b_ptr;       /* generic pointer to data */ \
                           daddr_t *__b_daddr;  /* data as disk block ptrs */ \
                           paddr_t b_paddr;     /* physical address */ \
                           uio_t *b_uio;        /* point to uio struct */ \
                         } b_un;

typedef struct giomap_buf {
	udi_ubit8_t		b_flags;	/* B_READ, B_WRITE */
	udi_ubit32_t		b_bcount;	/* Transfer length (bytes) */
	void			*b_addr;	/* Buffer address */
	udi_ubit32_t		b_resid;	/* Amount outstanding */
	udi_status_t		b_error;	/* Error code (status) */
	udi_ubit32_t		b_blkno;	/* Block # of start */
	udi_ubit32_t		b_blkoff;	/* Byte offset with block */
	_B_ADDR;
} giomap_buf_t;
#else
#define	giomap_buf_t	buf_t		/* Use OS buffer definitions */
#endif

typedef struct giomap_uio {
	void			*u_addr;	/* User buffer address */
	udi_ubit32_t		u_count;	/* Transfer length (bytes) */
	udi_status_t		u_error;	/* Error code (status) */
	udi_ubit32_t		u_resid;	/* Amount outstanding */
	udi_gio_op_t		u_op;		/* GIO operation */
	udi_ubit32_t		tr_param_len;	/* Length of tr_params */
	void			*tr_params;	/* User supplied tr_params */
	udi_boolean_t		u_async;	/* Asynchronous ioctl() */
	void			*u_handle;	/* For asynchronous requests */
} giomap_uio_t;

/*
 * Flag definitions
 */
#if UNIXWARE
#define	GIOMAP_B_READ		B_READ
#define	GIOMAP_B_WRITE		B_WRITE
#else
#define GIOMAP_B_READ		0x0001
#define GIOMAP_B_WRITE		0x0000
#endif

/*
 * OS-specific GIO ioctl values
 */
#define	GIOMAP_IOCTL(x)	(int)(((int)'G'<<24) | (x))

#define	UDI_GIO_DATA_XFER	GIOMAP_IOCTL(1)
#define	UDI_GIO_ABORT		GIOMAP_IOCTL(2)
#define	UDI_GIO_INQUIRY		GIOMAP_IOCTL(3)

/*
 * OS-specific maximum sizes
 */
#define	GIOMAP_BUFSIZE		65536		/* 64k maximum kernel buffer */
#define	GIOMAP_TR_PARAM_SZ	128		/* Maximum size of tr_params */
#define	GIOMAP_MAX_OFFSET	0xffff0000	/* Maximum offset before wrap*/

#define	GIOMAP_MAX_BLOCK	0x800000	/* Maximum 512-byte block no */
#define	GIOMAP_SEC_SHFT		9		/* Sector shift		     */

/*
 * Inter-mapper communications definitions
 */
#define	GIO_PASSTHRU		1		/* Pass-through channel	     */

typedef struct gio_map_req {
	void			*arg;		/* Return argument */
	void			*chanp;		/* Channel reference */
	int			oflags;		/* Open flags		*/
#if DDI
	cred_t			*credp;		/* Credentials		*/
	queue_t			*q;		/* Streams queue	*/
	rm_key_t		net_key;	/* RM key for net	*/
#endif
} gio_map_req_t;

#define	GIO_ADD_INSTANCE	0x1		/* Add new GIO instance */
#define	GIO_DEL_INSTANCE	0x2		/* Remove GIO instance  */
#define	GIO_OPEN_INSTANCE	0x3		/* pseudo open()	*/
#define	GIO_CLOSE_INSTANCE	0x4		/* pseudo close()	*/

