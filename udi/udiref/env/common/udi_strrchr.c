/*
 * File: src/env/common/udi_strrchr.c
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
#undef udi_strrchr

/*
 * udi_strrchr(), essentially ISO C strrchr.
 * s - src string pointer
 * c - character to find
 * Returns: Pointer to last occurance of c in string s, NULL if not found.
 */
char *
udi_strrchr(const char *s, char c)
{
	const char *found = NULL;

	/* We have to check the terminating '\0' */
	for ( ; ; s++) {
		if (*s == c)
			found = s;
		if (!*s)
			return (char *)found;
	}
}

