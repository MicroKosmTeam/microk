/*
 * File: src/env/common/udi_memmove.c
 *
 * UDI Environment -- Utilities: memory move.
 */

/*
 * Copyright (c) 2001  Kevin Van Maren.  All rights reserved.
 *
 * $Copyright udi_reference:
 * $
 */

#include <udi_env.h>
#undef udi_memmove

/*
 * udi_memmove(), essentially ISO C memmove.
 * d - destination buffer
 * s - source buffer
 * len - number of octets to move
 * Returns: d.
 *
 * This is the straightforward (slow) approach: please use an optimized
 * OSDEP version if possible.  (Or steal the bcopy code from *BSD or somewhere).
 */
void *
udi_memmove(void *d, const void *s, udi_size_t len)
{
	udi_ubit8_t *dst = (udi_ubit8_t *)d;
	udi_ubit8_t *src = (udi_ubit8_t *)s;

	/*
	 * Could "optimize" for 0-length and src==dst cases, but that
	 * would just slow down "real" calls.
	 */

	/*
	 * Since the ranges are allowed to overlap, we need to make sure
	 * that we don't nuke the data before we use it.
	 */
	if (dst < src) {
		/* Copy forward */
		while (len--) {
			*dst++ = *src++;
		}
	} else {
		/* Copy backwards */
		dst += len;
		src += len;
		while (len--) {
			*--dst = *--src;
		}
	}
	return (d);
}

