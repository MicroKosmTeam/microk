/*
 * File: driver/examples/hexdump/hexdump.c
 *
 * Example UDI "library" driver.
 */

/*
 *    Copyright (c) 2001  Kevin Van Maren.  All rights reserved.
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
 */

/*
 * udiref_hexdump() is a memory pretty-print routine.
 * We provide this functionality as a udi "library".
 */

#define UDI_VERSION 0x101
#include "udi.h"
#include "udiref_hexdump.h"

/* Not provided by UDI. */
#define isprint(c) ((c) >= ' ') && ((c) <= 126)


/*
 * Our buffer should be large enough, but being overly paranoid,
 * we use udi_snprintf() to ensure that we don't overflow it.
 * Calling udi_debug_printf() directly every time is certainly
 * simpler, but some environments in the past have added extra
 * output on udi_debug_printf(), so this approach is "safer":
 * the worst thing likely to occur with this approach is double-
 * spacing of the output.
 */
#define BUF_SIZE 80


/*
 * mem is the address of the buffer to dump, base is the starting address to display.
 * nbytes is the number of bytes of memory to pretty print.
 */
void
udiref_hexdump(void *mem, udi_size_t nbytes, void *base)
{
	int i;
	udi_ubit8_t *b = mem;
	char tbuf[BUF_SIZE];

	udi_debug_printf("/---------------------------------------------------------------------------\\\n");
	for (i = 0; i < nbytes; i += 16) {
		udi_size_t cnt, left;
		char *pbuf;
		int j;

		left = BUF_SIZE;
		pbuf = &tbuf[0];

		pbuf[0] = 0;

		/* assume 4 or 8-byte pointers */
		if (sizeof(void *) == 4) {
			cnt = udi_snprintf(pbuf, left, "| %08p      ",
					(char *)base + i);
		} else {
			cnt = udi_snprintf(pbuf, left, "| %016p  ",
					(char *)base + i);
		}
		pbuf = &pbuf[cnt];
		left -= cnt;

		/* Print the hex values */
		for (j = i; j < i + 16; j++) {

			if (j % 4 == 0) {
				cnt = udi_snprintf(pbuf, left, " ");
				pbuf = &pbuf[cnt];
				left -= cnt;
			}

			if (j < nbytes) {
				cnt = udi_snprintf(pbuf, left, "%02x", b[j]);
			} else {
				cnt = udi_snprintf(pbuf, left, "  ");
			}
			pbuf = &pbuf[cnt];
			left -= cnt;
		}

		/* Use extra padding if 4-byte pointers */
		if (sizeof(void *) == 4) {
			cnt = udi_snprintf(pbuf, left, "    ");
			pbuf = &pbuf[cnt];
			left -= cnt;
		}
		cnt = udi_snprintf(pbuf, left, "   ");
		pbuf = &pbuf[cnt];
		left -= cnt;

		/* Print the ascii characters */
		for (j = i; j < i + 16; j++) {
			if (j < nbytes) {
				cnt = udi_snprintf(pbuf, left, "%c",
						isprint(b[j]) ? b[j] : '.');
			} else {
				cnt = udi_snprintf(pbuf, left, " ");
			}
			pbuf = &pbuf[cnt];
			left -= cnt;
		}
		udi_snprintf(pbuf, left, " |");
		udi_debug_printf("%s\n", tbuf);
	}
	udi_debug_printf("\\---------------------------------------------------------------------------/\n");
}
