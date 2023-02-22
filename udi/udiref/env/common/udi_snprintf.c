
/*
 * File: env/common/udi_snprintf.c
 *
 * UDI Environment -- Utilities: Output Formatting Functions
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

/** Internal parameters and definitions **/

#define _UDI_NDIGITS 0x10		/* big enough to hold 64-bit addr */

#define _udi_is_endstring(Str) (*(Str) == 0)
#define _udi_addchar(C,T,L)    ((*((T)++) = (C)), (!--(L)))

udi_size_t
udi_vsnprintf(char *s,
	      udi_size_t max_bytes,
	      const char *format,
	      va_list argl)
{
    unsigned int used;
    const char *sp;
    char *dp;

    dp = s;
    for (sp = format; *sp && max_bytes; sp++) {
	if (*sp == '%') {

	    const char **src = &sp;
	    char **dst = &dp;
	    udi_size_t *dstlen = &max_bytes;

	    static const char Digits[] = "0123456789ABCDEF";
	    static const char digits[] = "0123456789abcdef";
	    static char nullstr[] = "<null>";
	    char tnbuf[_UDI_NDIGITS + 2];
	    char const *dp = digits;
	    char *s = (char *)*src;
	    char *d = *dst;
	    char *tp;
	    int ndig = 0, res = -1, lz = 0, lj = 0, dpset = 0, size = 4;
	    int ii = 10, argsused = 0, signed_val = 1;
	    int pz, pj;
	    udi_ubit32_t val;

	    /*
	     * Skip '%' 
	     */
	    s++;

	  try_again:

	    switch (*s) {

#if _UDI_PHYSIO_SUPPORTED
	    case 'A':
	    case 'a':
		/*
		 * Bus addresses are always 64 bits long and always hex.
		 * They also have a carnal knowledge of a udi_busaddr64_t.
		 */
		{
		    udi_busaddr64_t bus = va_arg(argl, udi_busaddr64_t);
		    udi_ubit32_t f32 = bus.first_32;
		    udi_ubit32_t l32 = bus.second_32;

		    dp = *s == 'A' ? Digits : digits;
		    tp = tnbuf;
		    ii = 16;
		    ndig = _UDI_NDIGITS;
		    argsused = 2;

		    /*
		     * If the high bits are nonzero, we can't blank suppress
		     * leading zeros in the low bits.
		     */
		    if (f32 != 0) {
			int i;

			for (i = 0; i < 8; l32 /= ii, i++)
			    if (_udi_addchar(dp[l32 % ii], tp, ndig))
				break;
		    } else {
			for (; l32; l32 /= ii)
			    if (_udi_addchar(dp[l32 % ii], tp, ndig))
				break;
		    }
		    for (; f32; f32 /= ii)
			if (_udi_addchar(dp[f32 % ii], tp, ndig))
			    break;
		    goto display;
		}
#endif

	    case 'P':
		dpset = 1;
		dp = Digits;
	    case 'p':
		if (!dpset) {
		    dpset = 1;
		}
		size = sizeof (void *);
	    case 'X':
		if (!dpset) {
		    dp = Digits;
		}
	    case 'x':
		ii = 0x10;
	    case 'u':
		signed_val = 0;
	    case 'd':
		argsused = 1;
		ndig = _UDI_NDIGITS;
		tp = tnbuf;

		switch (size) {
		case 4:
		    val = _UDI_VA_UBIT32_T(argl, udi_ubit32_t);

		    if (signed_val) {
			if ((udi_sbit32_t)val < 0) {
			    val = (udi_ubit32_t)(-(udi_sbit32_t)
						 val);
			    signed_val = -1;
			}
		    }
		    break;
		case 2:
		    val = _UDI_VA_UBIT16_T(argl, udi_ubit16_t);

		    if (signed_val) {
			if ((udi_sbit16_t)val < 0) {
			    val = (udi_ubit32_t)(-(udi_sbit16_t)
						 val);
			    signed_val = -1;
			}
		    }
		    break;
		case 1:
		    val = _UDI_VA_UBIT8_T(argl, udi_ubit8_t);

		    if (signed_val) {
			if ((udi_sbit8_t)val < 0) {
			    val = (udi_ubit32_t)(-(udi_sbit8_t)
						 val);
			    signed_val = -1;
			}
		    }
		    break;
		default:
		    /*
		     * If sizeof void* happens to not be "4", we land here.
		     * This is crafted to avoid #ifdefs even though it will
		     * not actually be executed on any target where sizeof 
		     * (void *) <=4.
		     */
		    if (size == sizeof (void *)) {
			unsigned long ptr = va_arg(argl,
						   unsigned long);

			for (; ptr; ptr /= ii)
			    if (_udi_addchar(dp[ptr % ii], tp, ndig))
				break;
			goto display;

		    } else {
			/*
			 * Should never happen 
			 */
			_OSDEP_ASSERT(0);
			;
		    }
		}

		for (; val; val /= ii)
		    if (_udi_addchar(dp[val % ii], tp, ndig))
			break;
		if (signed_val < 0) {
		    /*
		     * XXX - doesn't work properly w/ leading zeroes 
		     */
		    (void)_udi_addchar('-', tp, ndig);
		}
	      display:
		ndig -= _UDI_NDIGITS;
		if (!ndig) {
		    *tp++ = '0';
		    ndig--;
		}
		*tp = 0;
		tp = tnbuf;
		*src = s;
		break;

	    case 's':
	    case 'S':
		tp = _UDI_VA_POINTER(argl, char *);

		if (tp == NULL)
		    tp = nullstr;
		*src = s;
		argsused = 1;
		if (lj || res != -1) {
		    for (ndig = 0; ndig < *dstlen; ndig++)
			if (*(tp + ndig) == 0)
			    break;
		} else {
		    ndig = *dstlen;
		}
		break;

	    case 'c':
		*src = s;
		tp = tnbuf;
		*tp = (_UDI_VA_UBIT8_T(argl, char)) & 0xff;

		*(tp + 1) = 0;
		argsused = 1;
		ndig = 1;
		break;

	    case 'h':
		size = 2;
		s++;
		goto try_again;
	    case 'b':
		size = 1;
		s++;
		goto try_again;

	    case '0':
		if (s == *src + 1)
		    lz = 1;
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		res = (res == -1 ? 0 : res * 10) + *s - '0';
		s++;
		goto try_again;

	    case '-':
		lj = 1;
		s++;
		goto try_again;

		/*
		 * %<> takes 1 argument.  It is the numerical
		 * value to be interpreted.  The contents between the
		 * <> is an interpretation string whose BNR specification
		 * is:
		 *
		 *    istring = [BSDEL] bitspec [BSDEL bitspec] [BSDEL]
		 *    BSDEL = ','
		 *    BSNOT = '~'
		 *    BSVAL = '='
		 *    BSVID = ':'
		 *    BSRNG = '-'
		 *    bitspec = [BSNOT] bit-number BSVAL namestring  |
		 *              bit-num BSRNG bit-number BSVAL fieldnamestring
		 *    bit-num = decimal number 0-32
		 *    namestring = string to print
		 *    fieldnamestring = namestring [fieldspec ...]
		 *    fieldspec = BSVID fieldvalue BSVAL valuename
		 *    fieldvalue = decimal number >= 0
		 *    valuename = string to print if field == fieldvalue
		 *
		 * If bitspec is of the first form, then namestring is
		 * printed between BPSTART ('<') BPEND ('>') characters if
		 * the bit is set (or not set if BSNOT is specified) in the
		 * first numerical value and nothing is printed otherwise.
		 * If bitspec is of the second form, then the namestring is
		 * printed followed by BSVAL and then the hexadecimal value
		 * of the specified range of bits (the entire field again
		 * being enclosed by BPSTART, BPEND characters.
		 */
#define BSDEL ','
#define BSNOT '~'
#define BSVAL '='
#define BSVID ':'
#define BSRNG '-'
#define BFPSTART '<'			/* starts bitfield printing */
#define BPSTART ','			/* starts bit printing */
#define BPEND   '>'			/* ends bit printing */
#define BPIS    '='			/* bitfield value delimiter */

	    case BFPSTART:{
		    int bnum, negated, found;
		    const char *xlate, *xp;

		    val = _UDI_VA_UBIT32_T(argl, udi_ubit32_t);

		    xlate = s;
		    argsused = 1;
		    bnum = 0;
		    negated = 0;
		    pz = pj = 0;
		    for (xp = xlate; *xp != BPEND && *xp;
			 bnum = 0, negated = 0) {
			if (*xp == BFPSTART) {
			    (void)_udi_addchar(BFPSTART, d, *dstlen);
			    xp++;
			    continue;
			}
			if (*xp == BSDEL) {
			    xp++;
			    continue;
			}
			if (*xp == BSNOT) {
			    negated = 1;
			    xp++;
			}
			while (*xp != BSVAL && *xp != BSRNG)
			    bnum = (bnum * 10) + (*xp++ - '0');
			if (*xp == BSRNG) {
			    xp++;
			    pz = 0;
			    while (*xp != BSVAL)
				pz = (pz * 10) + (*xp++ - '0');
			    for (ndig = 0; pz >= bnum; pz--)
				ndig |= (1 << pz);
			    if (!pj) {
				pj = 1;
			    } else {
				if (_udi_addchar(BPSTART, d, *dstlen))
				    goto end_copy;
			    }
			    xp++;
			    while (*xp != BSDEL &&
				   !_udi_is_endstring
				   (xp) && *xp != BSVID && *xp != BPEND)
				if (_udi_addchar(*xp++, d, *dstlen))
				    goto end_copy;
			    pz = (val & ndig) >> bnum;
			    if (*xp == BSVID) {
				xp++;
				found = 0;
				while (*xp !=
				       BSDEL &&
				       *xp !=
				       BPEND && !_udi_is_endstring(xp)) {
				    bnum = 0;
				    while (*xp != BSVAL && *xp != BSVID)
					bnum = (bnum * 10) + (*xp++ - '0');
				    if (pz == bnum) {
					found = 1;
					while (*xp != BSVID && *xp != BSDEL &&
					       *xp != BPEND &&
					       !_udi_is_endstring(xp))
					    if (_udi_addchar
						(*xp++, d, *dstlen))
						goto end_copy;
				    } else {
					while (*xp != BSVID && *xp != BSDEL &&
					       *xp != BPEND &&
					       !_udi_is_endstring(xp)) {
					    xp++;
					}
				    }
				    if (*xp == BSVID) {
					xp++;
				    }
				}
				if (!found) {
				    if (_udi_addchar(BPIS, d, *dstlen))
					goto end_copy;
				    d += udi_snprintf(d, *dstlen, "%X", pz);
				}
			    } else {
				if (_udi_addchar(BPIS, d, *dstlen))
				    goto end_copy;
				d += udi_snprintf(d, *dstlen, "%X", pz);
			    }
			} else {
			    xp++;
			    if (((negated == 0) &&
				 (bnum <
				  sizeof (val) * 8 && val & (1 << bnum)))
				|| ((negated == 1)
				    && (bnum <
					sizeof
					(val) * 8 && ((val & (1 << bnum))
						      == 0)))) {
				if (!pj) {
				    pj = 1;
				} else {
				    if (_udi_addchar(BPSTART, d, *dstlen))
					goto end_copy;
				}
				while (*xp !=
				       BSDEL &&
				       *xp != BPEND && !_udi_is_endstring(xp))
				    if (_udi_addchar(*xp++, d, *dstlen))
					goto end_copy;
			    } else {
				while (*xp !=
				       BSDEL &&
				       *xp != BPEND && !_udi_is_endstring(xp))
				    xp++;
			    }
			}
		    }
		    (void)_udi_addchar(*xp, d, *dstlen);
		    *dst = d;
		    *src = xp;
		    used = 1;
		    goto _convert_arg_end;
		}

	    default:
		*src = tp = s;
		ndig = 1;
		break;
	    }

	    ii = ndig;
	    if (ndig < 0)
		ndig = -ndig;
	    if (res == -1)
		res = ndig;

	    if (lj) {
		lj = res - ndig;
		if (lj < 0)
		    lj = 0;
		if (res-- > ndig)
		    res = ndig;
	    } else {
		while (res-- > ndig)
		    if (_udi_addchar((lz ? '0' : ' '), d, *dstlen))
			goto end_copy;
	    }

	    if (ndig > ++res) {
		ndig = res;
	    }

	    if (ii < 0) {
		ii = ndig - 1;
		res = -1;
	    } else {
		ii = 0;
		res = 1;
	    }

	    while (ndig-- && tp[ii]) {
		if (_udi_addchar(tp[ii], d, *dstlen))
		    goto end_copy;
		ii += res;
	    }

	    if (lj && *dstlen) {
		while (lj--)
		    if (_udi_addchar(' ', d, *dstlen))
			goto end_copy;
	    }

	    (void)_udi_addchar(0, d, *dstlen);

	  end_copy:
	    *dst = --d;
	    used = argsused;
	  _convert_arg_end:
	    continue;
	}
	if (_udi_addchar(*sp, dp, max_bytes))
	    break;
    }
    *dp = 0;
    return dp - s;
}

udi_size_t
udi_snprintf(char *s,
	     udi_size_t max_bytes,
	     const char *format,
	     ...)
{
    va_list arg;
    udi_size_t rval;

    va_start(arg, format);
    rval = udi_vsnprintf(s, max_bytes, format, arg);
    va_end(arg);

    return rval;
}
