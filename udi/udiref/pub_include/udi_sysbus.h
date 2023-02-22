/*
 * File: udi_sysbus.h
 *
 * UDI System Bus Binding definitions.
 *
 * All UDI drivers that require System Bus bindings must #include this file.
 *
 * 
 *    Copyright (c) 2001; The Santa Cruz Operation, Inc.
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

#ifndef _UDI_SYSBUS_H
#define _UDI_SYSBUS_H
/*
 * Validate UDI_PHYSIO_VERSION.
 */
#if !defined(UDI_SYSBUS_VERSION)
#error "UDI_SYSBUS_VERSION must be defined."
#elif (UDI_VERSION != 0x101)
#error "Unsupported UDI_SYSBUS_VERSION."
#endif

/*
 * Validate #include dependencies.
 */
#ifndef _UDI_H
#error "udi.h must be #included before udi_physio.h."
#endif

#ifndef _UDI_PHYSIO_H
#error "udi_physio.h must be #included before udi_sysbus.h"
#endif

/*
 * Regset indices for udi_pio_map.
 */

#define UDI_SYSBUS_MEM 		0
#define UDI_SYSBUS_IO 		1 
#define UDI_SYSBUS_MEM_BUS	2 
#define UDI_SYSBUS_IO_BUS	3


#endif /* _UDI_PHYSIO_H */
