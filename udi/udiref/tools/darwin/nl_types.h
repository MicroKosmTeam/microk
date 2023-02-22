/* This file exists cause Darwin doesn't have nl_types.h. */
#ifndef __NL_TYPES_H__
#define __NL_TYPES_H__
#define catopen(filename, locale) 0
#define catgets(x,y,z,s) s
#define catclose(stuff)
typedef int nl_catd;
#endif
