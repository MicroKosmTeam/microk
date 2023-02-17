
/*
 * File: env/common/udi_sprops_ra.c
 *
 * UDI Static driver properties definitions.
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

/* Region Attributes within static properties */
#define _UDI_RA_TYPE_MASK	(0x00000007)
#define _UDI_RA_NORMAL		(1<<0)
#define _UDI_RA_FP		(1<<1)
#define _UDI_RA_INTERRUPT	(1<<2)

#define _UDI_RA_BINDING_MASK	(0x00000030)
#define _UDI_RA_STATIC		(1<<4)
#define _UDI_RA_DYNAMIC		(1<<5)

#define _UDI_RA_PRIORITY_MASK	(0x00000700)
#define _UDI_RA_LO		(1<<8)
#define _UDI_RA_MED		(1<<9)
#define _UDI_RA_HI		(1<<10)

#define _UDI_RA_LATENCY_MASK	(0x001F0000)
#define _UDI_RA_POWER_WARN	(1<<16)
#define _UDI_RA_OVERRUN		(1<<17)
#define _UDI_RA_RETRY		(1<<18)
#define _UDI_RA_NON_OVERRUN	(1<<19)
#define _UDI_RA_NON_CRITICAL	(1<<20)

#define _UDI_RA_PIO_PROBE	(1<<30)
#define _UDI_RA_PIO_SERIAL_LIMIT	(1U<<31)
