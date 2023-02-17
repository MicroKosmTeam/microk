/*
 * File: src/env/common/udi_memchr.c
 *
 * UDI Environment -- Utilities: String/Memory
 */

/*
 * Copyright (c) 2001  Kevin Van Maren.  All rights reserved.
 *
 * $Copyright udi_reference:
 * $
 */

#include <udi_env.h>
#undef udi_memchr

/*
 * udi_memchr(), essentially ISO C memchr.
 * src - buffer to seatch
 * c - `char' to find
 * len - size of buffer
 * Returns: pointer to the first occurance of c, NULL if not found.
 */
void *
udi_memchr(const void *src, udi_ubit8_t c, udi_size_t len)
{
	const udi_ubit8_t *s;

	for (s = src; len; s++, len--)
		if (*s == c)
			return (void *) s;
	return NULL;
}

