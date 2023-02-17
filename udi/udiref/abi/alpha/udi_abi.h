/*
 * File: alpha/udi_abi.h
 *
 * ABI definitions for this ABI binding: alpha
 *
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

#ifdef _UDI_ABI_H
#if _UDI_ABI_H != "alpha"
#error "UDI ABI already defined and not as alpha!"
#endif
#else

#define _UDI_ABI_H "alpha"

#define UDI_ABI_NAME        "alpha"
#define UDI_ABI_PROCESSORS  "Compaq Alpha"
#define UDI_ABI_ENDIANNESS  "little"
#define UDI_ABI_OBJFMT      "ELF"
#define UDI_ABI_UDIPROPS    "ELF .udiprops section"

#define UDI_ABI_ELF_UDIPROPS_SECTION

#define UDI_ABI_VOIDP_SIZE       64
#define UDI_ABI_SIZET_SIZE       64
#define UDI_ABI_INDEXT_SIZE      8
#define UDI_ABI_BUF_SIZE         64
#define UDI_ABI_CHANNEL_SIZE     64
#define UDI_ABI_CONSTRAINTS_SIZE 64
#define UDI_ABI_TIMESTAMP_SIZE   64
#define UDI_ABI_PIO_HANDLE_SIZE  64
#define UDI_ABI_DMA_HANDLE_SIZE  64

#ifndef KBytes
#define KBytes *1024
#endif
#ifndef MBytes
#define MBytes *1024*1024
#endif

#define UDI_ABI_MAX_CALL_STACK   (16 KBytes)
#define UDI_ABI_MAX_MOD_CODE     (64 KBytes)
#define UDI_ABI_MAX_MOD_DATA     (64 KBytes)
#define UDI_ABI_MAX_MOD_FILE     (256 KBytes)
#define UDI_ABI_MAX_DRV_CODE     (2 MBytes)
#define UDI_ABI_MAX_DRV_DATA     (2 MBytes)

#define UDI_HANDLE_ID(handle, handle_type)  ((void *)(handle))
#define UDI_HANDLE_IS_NULL(handle)          ((void *)(handle) == NULL)

extern void _udi_assert(int result,
			char *test,
			char *filename,
			char *linenum);

#define udi_assert(X)  { if (!(X)) _udi_assert(X, #X, __FILE__, __LINE__); }

#endif /* _UDI_ABI_H */
