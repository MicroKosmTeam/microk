
/*
 * File: env/common/udi_endian.c
 *
 * UDI Environment -- Endianness Functions
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

/*
 * This tends to be written for optimizers, not for humans.
 * I analyzed the (optimized) compiler output from three compilers on 
 * two architectures for having separate functions for the in-place and 
 * non-overlapping copy cases but the difference in the performance-critical 
 * cases (16, 32 bits) was zero and the difference in the "big" case 
 * was minor (one register has to be spilled in the loop).
 *
 * There are a variety of architecture and compiler specific optimizations 
 * that can be applied, but this set of swapping utilities is a reasonable
 * compromise between good performance and portability.
 */

void
udi_endian_swap(const void *src,
		void *dst,
		udi_ubit8_t swap_size,
		udi_ubit8_t stride,
		udi_ubit16_t rep_count)
{
	/*
	 * Verify programmer passed power of 2, per spec. 
	 * (Remember that zero isn't a power of two either.)
	 */
	_OSDEP_ASSERT((swap_size & (swap_size - 1)) == 0);

	switch (swap_size) {
	case 1:
		return;
	case 2:{
			udi_ubit16_t *u16p = (udi_ubit16_t *)src;
			udi_ubit16_t *u16dp = (udi_ubit16_t *)dst;

			stride /= 2;
			while (rep_count--) {
				*u16dp = _OSDEP_ENDIAN_SWAP_16(*u16p);
				u16p = &u16p[stride];
				u16dp = &u16dp[stride];
			}
			break;
		}

	case 4:{
			udi_ubit32_t *u32p = (udi_ubit32_t *)src;
			udi_ubit32_t *u32dp = (udi_ubit32_t *)dst;

			stride /= 4;
			while (rep_count--) {
				*u32dp = _OSDEP_ENDIAN_SWAP_32(*u32p);
				u32p = &u32p[stride];
				u32dp = &u32dp[stride];
			}
			break;
		}

		/*
		 * If it isn't something we can highly optimize, just
		 * do a manual byte copy of it, swapping them around the 
		 * pivot point as we go.   This takes advantage of the 
		 * rule that our count has to be a power of two.  (If it
		 * were to be odd, for example, we'd be hosed.)
		 */
	default:
		while (rep_count--) {
			udi_ubit8_t const *p = src;	/* ptr to source */
			udi_ubit8_t const *p2;	/* tgt partner of this flip */
			udi_ubit8_t *pd = dst;	/* ptr to destination */
			udi_ubit8_t *p2d;	/* tgt partner of this flip */

			p += rep_count * stride;
			pd += rep_count * stride;
			p2 = p + swap_size;
			p2d = pd + swap_size;

			while (p != p2) {
				udi_ubit8_t temp = *p++;

				*pd++ = *--p2;
				*--p2d = temp;
			}
		}
	}
}
