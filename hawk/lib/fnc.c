/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "hawk-prv.h"

static int fnc_close (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi);
static int fnc_fflush (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi);
static int fnc_int (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi);
static int fnc_typename (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi);
static int fnc_gcrefs (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi);
static int fnc_isnil (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi);
static int fnc_ismap (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi);
static int fnc_asort (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi);
static int fnc_asorti (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi);

#define A_MAX HAWK_TYPE_MAX(int)

/* Argument Specifier 
 *
 * Each character in the specifier indicates how a parameter
 * of the corresponding postion should be passed to a function.
 *
 * - v: value. pass it after normal evaluation.
 * - r: pass a variable by reference
 * - x: regular expression as it is. not evaluated as /rex/ ~ $0.
 * 
 * NOTE: If min is greater than max, the specifier indicate the 
 * name of the module  where the function is located.
 */
static hawk_fnc_t sysfnctab[] = 
{
	/* io functions */
	{ {HAWK_T("close"),    5}, 0, { {1,     2, HAWK_NULL},        fnc_close,  HAWK_RIO }, HAWK_NULL},
	{ {HAWK_T("fflush"),   6}, 0, { {0,     1, HAWK_NULL},        fnc_fflush,  HAWK_RIO }, HAWK_NULL},

	/* type info/conversion */
	{ {HAWK_T("int"),      3}, 0, { {1,     1, HAWK_NULL},        fnc_int,              0 }, HAWK_NULL},
	{ {HAWK_T("isnil"),    5}, 0, { {1,     1, HAWK_NULL},        fnc_isnil,            0 }, HAWK_NULL},
	{ {HAWK_T("ismap"),    5}, 0, { {1,     1, HAWK_NULL},        fnc_ismap,            0 }, HAWK_NULL},
	{ {HAWK_T("typename"), 8}, 0, { {1,     1, HAWK_NULL},        fnc_typename,         0 }, HAWK_NULL},
	{ {HAWK_T("gcrefs"),   6}, 0, { {1,     1, HAWK_NULL},        fnc_gcrefs,           0 }, HAWK_NULL},

	/* map(array) sort */
	{ {HAWK_T("asort"),    5}, 0, { {1,     3, HAWK_T("rrv")},    fnc_asort,            0 }, HAWK_NULL},
	{ {HAWK_T("asorti"),   6}, 0, { {1,     3, HAWK_T("rrv")},    fnc_asorti,           0 }, HAWK_NULL},
 
	/* string functions */
	{ {HAWK_T("gsub"),     4}, 0, { {2,     3, HAWK_T("xvr")},    hawk_fnc_gsub,     0 }, HAWK_NULL},
	{ {HAWK_T("index"),    5}, 0, { {2,     3, HAWK_NULL},        hawk_fnc_index,    0 }, HAWK_NULL},
	{ {HAWK_T("length"),   6}, 1, { {0,     1, HAWK_NULL},        hawk_fnc_length,   0 }, HAWK_NULL},
	{ {HAWK_T("match"),    5}, 0, { {2,     4, HAWK_T("vxvr")},   hawk_fnc_match,    0 }, HAWK_NULL},
	{ {HAWK_T("split"),    5}, 0, { {2,     3, HAWK_T("vrx")},    hawk_fnc_split,    0 }, HAWK_NULL},
	{ {HAWK_T("sprintf"),  7}, 0, { {1, A_MAX, HAWK_NULL},        hawk_fnc_sprintf,  0 }, HAWK_NULL},
	{ {HAWK_T("sub"),      3}, 0, { {2,     3, HAWK_T("xvr")},    hawk_fnc_sub,      0 }, HAWK_NULL},
	{ {HAWK_T("substr"),   6}, 0, { {2,     3, HAWK_NULL},        hawk_fnc_substr,   0 }, HAWK_NULL},
	{ {HAWK_T("tolower"),  7}, 0, { {1,     1, HAWK_NULL},        hawk_fnc_tolower,  0 }, HAWK_NULL},
	{ {HAWK_T("toupper"),  7}, 0, { {1,     1, HAWK_NULL},        hawk_fnc_toupper,  0 }, HAWK_NULL},

	/* math functions */
	{ {HAWK_T("sin"),      3}, 0, { {A_MAX, 0, HAWK_T("math") },  HAWK_NULL,             0 }, HAWK_NULL},
	{ {HAWK_T("cos"),      3}, 0, { {A_MAX, 0, HAWK_T("math") },  HAWK_NULL,             0 }, HAWK_NULL},
	{ {HAWK_T("tan"),      3}, 0, { {A_MAX, 0, HAWK_T("math") },  HAWK_NULL,             0 }, HAWK_NULL},
	{ {HAWK_T("atan"),     4}, 0, { {A_MAX, 0, HAWK_T("math") },  HAWK_NULL,             0 }, HAWK_NULL},
	{ {HAWK_T("atan2"),    5}, 0, { {A_MAX, 0, HAWK_T("math") },  HAWK_NULL,             0 }, HAWK_NULL},
	{ {HAWK_T("log"),      3}, 0, { {A_MAX, 0, HAWK_T("math") },  HAWK_NULL,             0 }, HAWK_NULL},
	{ {HAWK_T("log10"),    5}, 0, { {A_MAX, 0, HAWK_T("math") },  HAWK_NULL,             0 }, HAWK_NULL},
	{ {HAWK_T("exp"),      3}, 0, { {A_MAX, 0, HAWK_T("math") },  HAWK_NULL,             0 }, HAWK_NULL},
	{ {HAWK_T("sqrt"),     4}, 0, { {A_MAX, 0, HAWK_T("math") },  HAWK_NULL,             0 }, HAWK_NULL},

	/* time functions */
	{ {HAWK_T("mktime"),   6}, 0, { {A_MAX, 0, HAWK_T("sys") },   HAWK_NULL,             0 }, HAWK_NULL},
	{ {HAWK_T("strftime"), 8}, 0, { {A_MAX, 0, HAWK_T("sys") },   HAWK_NULL,             0 }, HAWK_NULL},
	{ {HAWK_T("systime"),  7}, 0, { {A_MAX, 0, HAWK_T("sys") },   HAWK_NULL,             0 }, HAWK_NULL}
};

static hawk_fnc_t* add_fnc (hawk_t* hawk, const hawk_ooch_t* name, const hawk_fnc_spec_t* spec)
{
	hawk_fnc_t* fnc;
	hawk_oow_t fnc_size;
	hawk_oow_t speclen;
	hawk_oocs_t ncs;

	ncs.ptr = (hawk_ooch_t*)name;
	ncs.len = hawk_count_oocstr(name);
	if (ncs.len <= 0)
	{
		hawk_seterrnum (hawk, HAWK_NULL, HAWK_EINVAL);
		return HAWK_NULL;
	}

	/* Note it doesn't check if it conflicts with a keyword.
	 * such a function registered won't take effect because
	 * the word is treated as a keyword */

	if (hawk_findfncwithoocs(hawk, &ncs) != HAWK_NULL)
	{
		hawk_seterrfmt (hawk, HAWK_NULL, HAWK_EEXIST, HAWK_T("unable to add existing function - %js"), name);
		return HAWK_NULL;
	}

	speclen = spec->arg.spec? hawk_count_oocstr(spec->arg.spec): 0;

	fnc_size = HAWK_SIZEOF(*fnc) + (ncs.len + 1 + speclen + 1) * HAWK_SIZEOF(hawk_ooch_t);
	fnc = (hawk_fnc_t*)hawk_callocmem(hawk, fnc_size);
	if (fnc)
	{
		hawk_ooch_t* tmp;

		tmp = (hawk_ooch_t*)(fnc + 1);
		fnc->name.len = hawk_copy_oocstr_unlimited(tmp, ncs.ptr);
		fnc->name.ptr = tmp;

		fnc->spec = *spec;
		if (spec->arg.spec)
		{
			tmp = fnc->name.ptr + fnc->name.len + 1;
			hawk_copy_oocstr_unlimited (tmp, spec->arg.spec); 
			fnc->spec.arg.spec = tmp;
		}

		if (hawk_htb_insert(hawk->fnc.user, (hawk_ooch_t*)ncs.ptr, ncs.len, fnc, 0) == HAWK_NULL)
		{
			const hawk_ooch_t* bem = hawk_backuperrmsg(hawk);
			hawk_seterrfmt (hawk, HAWK_NULL, hawk_geterrnum(hawk), HAWK_T("unable to add function - %js - %js"), name, bem);
			hawk_freemem (hawk, fnc);
			fnc = HAWK_NULL;
		}
	}

	return fnc;
}

hawk_fnc_t* hawk_addfncwithbcstr (hawk_t* awk, const hawk_bch_t* name, const hawk_fnc_mspec_t* spec)
{
#if defined(HAWK_OOCH_IS_BCH)
	return add_fnc(awk, name, spec);
#else
	hawk_ucs_t wcs;
	hawk_fnc_t* fnc;
	hawk_fnc_spec_t wspec;

	HAWK_STATIC_ASSERT (HAWK_SIZEOF(*spec) == HAWK_SIZEOF(wspec));

	HAWK_MEMCPY (&wspec, spec, HAWK_SIZEOF(wspec));
	if (spec->arg.spec)
	{
		wcs.ptr = hawk_dupbtoucstr(awk, spec->arg.spec, &wcs.len, 0);
		if (HAWK_UNLIKELY(!wcs.ptr)) return HAWK_NULL;
		wspec.arg.spec = wcs.ptr;
	}

	wcs.ptr = hawk_dupbtoucstr(awk, name, &wcs.len, 0);
	if (HAWK_UNLIKELY(!wcs.ptr)) 
	{
		if (wspec.arg.spec) hawk_freemem (awk, (hawk_uch_t*)wspec.arg.spec);
		return HAWK_NULL;
	}

	fnc = add_fnc(awk, wcs.ptr, &wspec);
	hawk_freemem (awk, wcs.ptr);
	if (wspec.arg.spec) hawk_freemem (awk, (hawk_uch_t*)wspec.arg.spec);
	return fnc;
#endif
}

hawk_fnc_t* hawk_addfncwithucstr (hawk_t* awk, const hawk_uch_t* name, const hawk_fnc_wspec_t* spec)
{
#if defined(HAWK_OOCH_IS_BCH)
	hawk_bcs_t mbs;
	hawk_fnc_t* fnc;
	hawk_fnc_spec_t mspec;

	HAWK_STATIC_ASSERT (HAWK_SIZEOF(*spec) == HAWK_SIZEOF(mspec));

	HAWK_MEMCPY (&mspec, spec, HAWK_SIZEOF(mspec));
	if (spec->arg.spec)
	{
		mbs.ptr = hawk_duputobcstr(awk, spec->arg.spec, &mbs.len);
		if (HAWK_UNLIKELY(!mbs.ptr)) return HAWK_NULL;
		mspec.arg.spec = mbs.ptr;
	}

	mbs.ptr = hawk_duputobcstr(awk, name, &mbs.len);
	if (HAWK_UNLIKELY(!mbs.ptr))
	{
		if (mspec.arg.spec) hawk_freemem (awk, (hawk_bch_t*)mspec.arg.spec);
		return HAWK_NULL;
	}

	fnc = add_fnc(awk, mbs.ptr, &mspec);
	hawk_freemem (awk, mbs.ptr);
	if (mspec.arg.spec) hawk_freemem (awk, (hawk_bch_t*)mspec.arg.spec);
	return fnc;
#else
	return add_fnc(awk, name, spec);
#endif
}

int hawk_delfncwithbcstr (hawk_t* hawk, const hawk_bch_t* name)
{
	hawk_bcs_t ncs;
	hawk_ucs_t wcs;

	ncs.ptr = (hawk_bch_t*)name;
	ncs.len = hawk_count_bcstr(name);

#if defined(HAWK_OOCH_IS_BCH)
	if (hawk_htb_delete(hawk->fnc.user, ncs.ptr, ncs.len) <= -1)
	{
		hawk_seterrfmt (hawk, HAWK_NULL, HAWK_ENOENT, HAWK_T("no such function - %hs"), name);
		return -1;
	}
#else
	wcs.ptr = hawk_dupbtoucstr(hawk, ncs.ptr, &wcs.len, 0);
	if (!wcs.ptr) return -1;
	if (hawk_htb_delete(hawk->fnc.user, wcs.ptr, wcs.len) <= -1)
	{
		hawk_seterrfmt (hawk, HAWK_NULL, HAWK_ENOENT, HAWK_T("no such function - %hs"), name);
		hawk_freemem (hawk, wcs.ptr);
		return -1;
	}
	hawk_freemem (hawk, wcs.ptr);
#endif

	return 0;
}

int hawk_delfncwithucstr (hawk_t* hawk, const hawk_uch_t* name)
{
	hawk_ucs_t ncs;
	hawk_bcs_t mbs;

	ncs.ptr = (hawk_uch_t*)name;
	ncs.len = hawk_count_ucstr(name);

#if defined(HAWK_OOCH_IS_BCH)
	mbs.ptr = hawk_duputobcstr(hawk, ncs.ptr, &mbs.len);
	if (!mbs.ptr) return -1;
	if (hawk_htb_delete(hawk->fnc.user, mbs.ptr, mbs.len) <= -1)
	{
		hawk_seterrfmt (hawk, HAWK_NULL, HAWK_ENOENT, HAWK_T("no such function - %ls"), name);
		hawk_freemem (hawk, mbs.ptr);
		return -1;
	}
	hawk_freemem (hawk, mbs.ptr);
#else
	if (hawk_htb_delete(hawk->fnc.user, ncs.ptr, ncs.len) <= -1)
	{
		hawk_seterrfmt (hawk, HAWK_NULL, HAWK_ENOENT, HAWK_T("no such function - %ls"), name);
		return -1;
	}
#endif

	return 0;
}

void hawk_clrfnc (hawk_t* awk)
{
	hawk_htb_clear (awk->fnc.user);
}

static hawk_fnc_t* find_fnc (hawk_t* awk, const hawk_oocs_t* name)
{
	hawk_htb_pair_t* pair;
	int i;

	/* search the system function table 
	 * though some optimization like binary search can
	 * speed up the search, i don't do that since this
	 * function is called durting parse-time only. 
	 */
	for (i = 0; i < HAWK_COUNTOF(sysfnctab); i++)
	{
		if ((awk->opt.trait & sysfnctab[i].spec.trait) != sysfnctab[i].spec.trait) continue;
		if (hawk_comp_oochars(sysfnctab[i].name.ptr, sysfnctab[i].name.len, name->ptr, name->len, 0) == 0) return &sysfnctab[i];
	}

	pair = hawk_htb_search(awk->fnc.user, name->ptr, name->len);
	if (pair)
	{
		hawk_fnc_t* fnc;
		fnc = (hawk_fnc_t*)HAWK_HTB_VPTR(pair);
		if ((awk->opt.trait & fnc->spec.trait) == fnc->spec.trait) return fnc;
	}

	hawk_seterrfmt (awk, HAWK_NULL, HAWK_ENOENT, HAWK_T("no such function - %js"), name);
	return HAWK_NULL;
}

hawk_fnc_t* hawk_findfncwithbcs (hawk_t* awk, const hawk_bcs_t* name)
{
#if defined(HAWK_OOCH_IS_BCH)
	return find_fnc(awk, name);
#else
	hawk_ucs_t wcs;
	hawk_fnc_t* fnc;

	wcs.ptr = hawk_dupbtouchars(awk, name->ptr, name->len, &wcs.len, 0);
	if (HAWK_UNLIKELY(!wcs.ptr)) return HAWK_NULL;
	fnc = find_fnc(awk, &wcs);
	hawk_freemem (awk, wcs.ptr);
	return fnc;
#endif
}

hawk_fnc_t* hawk_findfncwithucs (hawk_t* awk, const hawk_ucs_t* name)
{
#if defined(HAWK_OOCH_IS_BCH)
	hawk_bcs_t mbs;
	hawk_fnc_t* fnc;

	mbs.ptr = hawk_duputobchars(awk, name->ptr, name->len, &mbs.len);
	if (HAWK_UNLIKELY(!mbs.ptr)) return HAWK_NULL;
	fnc = find_fnc(awk, &mbs);
	hawk_freemem (awk, mbs.ptr);
	return fnc;
#else
	return find_fnc(awk, name);
#endif
}

static int fnc_close (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_oow_t nargs;
	hawk_val_t* v, * a0, * a1 = HAWK_NULL;
	int n;

	hawk_ooch_t* name, * opt = HAWK_NULL;
	hawk_oow_t len, optlen = 0;
       
	nargs = hawk_rtx_getnargs(rtx);
	HAWK_ASSERT (nargs == 1 || nargs == 2);

	a0 = hawk_rtx_getarg (rtx, 0);
	if (nargs >= 2) a1 = hawk_rtx_getarg(rtx, 1);
	HAWK_ASSERT (a0 != HAWK_NULL);

	name = hawk_rtx_getvaloocstr(rtx, a0, &len);
	if (name == HAWK_NULL) return -1;

	if (a1)
	{
		opt = hawk_rtx_getvaloocstr(rtx, a1, &optlen);
		if (opt == HAWK_NULL) 
		{
			hawk_rtx_freevaloocstr(rtx, a0, name);
			return -1;
		}
	}

	if (len == 0)
	{
		/* getline or print doesn't allow an empty string for the 
		 * input or output file name. so close should not allow 
		 * it either.  
		 * another reason for this is if close is called explicitly 
		 * with an empty string, it may close the console that uses 
		 * an empty string for its identification because closeio
		 * closes any ios that match the name given unlike 
		 * closeio_read or closeio_write. */ 
		n = -1;
		goto skip_close;
	}

	while (len > 0)
	{
		if (name[--len] == HAWK_T('\0'))
		{
			/* the name contains a null charater. 
			 * make close return -1 */
			n = -1;
			goto skip_close;
		}
	}	

	if (opt)
	{
		if (optlen != 1 || (opt[0] != HAWK_T('r') && opt[0] != HAWK_T('w')))
		{
			n = -1;
			goto skip_close;
		}
	}

	n = hawk_rtx_closeio (rtx, name, opt);
	/* failure to close is not a critical error. instead, that is
	 * flagged by the return value of close(). 
	if (n <= -1 && rtx->errinf.num != HAWK_EIONMNF)
	{
		if (a0->type != HAWK_VAL_STR) hawk_rtx_freemem (rtx, name);
		return -1;
	}
	*/

skip_close:
	if (a1) hawk_rtx_freevaloocstr (rtx, a1, opt);
	hawk_rtx_freevaloocstr (rtx, a0, name);

	v = hawk_rtx_makeintval (rtx, (hawk_int_t)n);
	if (v == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, v);
	return 0;
}

static int flush_io (hawk_rtx_t* rtx, int rio, const hawk_ooch_t* name, int n)
{
	int n2;

	if (rtx->rio.handler[rio] != HAWK_NULL)
	{
		n2 = hawk_rtx_flushio (rtx, rio, name);
		if (n2 <= -1)
		{
			/*
			if (rtx->errinf.num == HAWK_EIOIMPL) n = -1;
			else if (rtx->errinf.num == HAWK_EIONMNF) 
			{
				if (n != 0) n = -2;
			}
			else n = -99; 
			*/	
			if (hawk_rtx_geterrnum(rtx) == HAWK_EIONMNF) 
			{
				if (n != 0) n = -2;
			}
			else n = -1;
		}
		else if (n != -1) n = 0;
	}

	return n;
}

static int fnc_fflush (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_oow_t nargs;
	hawk_val_t* a0, * v;
	hawk_ooch_t* str0;
	hawk_oow_t len0;
	int n;

	nargs = hawk_rtx_getnargs (rtx);
	HAWK_ASSERT (nargs == 0 || nargs == 1);

	if (nargs == 0)
	{
		/* fflush() flushes the console output.
		 * fflush() should return -1 on errors.
		 *
		 * if no previous console output statement is seen,
		 * this function won't be able to find the entry.
		 * so it returns -1;
		 *
		 * BEGIN { flush(); } # flush() returns -1
		 * BEGIN { print 1; flush(); } # flush() returns 0
		 */
		n = hawk_rtx_flushio (rtx, HAWK_OUT_CONSOLE, HAWK_T(""));
	}
	else
	{
		hawk_ooch_t* ptr, * end;

		a0 = hawk_rtx_getarg (rtx, 0);
		str0 = hawk_rtx_getvaloocstr (rtx, a0, &len0);
		if (str0 == HAWK_NULL) return -1;

		/* the target name contains a null character.
		 * make fflush return -1 */
		ptr = str0; end = str0 + len0;
		while (ptr < end)
		{
			if (*ptr == HAWK_T('\0')) 
			{
				n = -1;
				goto skip_flush;
			}

			ptr++;
		}

		/* flush the given rio.
		 *
		 * flush("") flushes all output streams regardless of names.
		 * pass HAWK_NULL for the name in that case so that the
		 * callee matches any streams. 
		 *
		 * fflush() doesn't specify the type of output streams
		 * so it attemps to flush all types of output streams.
		 * 
		 * though not useful, it's possible to have multiple
		 * streams with the same name but of different types.
		 * 
		 *  BEGIN { 
		 *    print 1 | "/tmp/x"; 
		 *    print 1 > "/tmp/x";
		 *    fflush("/tmp/x"); 
		 *  }
		 */

		n = flush_io (
			rtx, HAWK_OUT_FILE, 
			((len0 == 0)? HAWK_NULL: str0), 1);
		/*if (n == -99) return -1;*/
		n = flush_io (
			rtx, HAWK_OUT_APFILE, 
			((len0 == 0)? HAWK_NULL: str0), n);
		/*if (n == -99) return -1;*/
		n = flush_io (
			rtx, HAWK_OUT_PIPE,
			((len0 == 0)? HAWK_NULL: str0), n);
		/*if (n == -99) return -1;*/
		n = flush_io (
			rtx, HAWK_OUT_RWPIPE,
			((len0 == 0)? HAWK_NULL: str0), n);
		/*if (n == -99) return -1;*/

		/* if n remains 1, no io handlers have been defined for
		 * file, pipe, and rwpipe. so make fflush return -1. 
		 * if n is -2, no such named io has been found at all 
		 * if n is -1, the io handler has returned an error */
		if (n != 0) n = -1;

	skip_flush:
		hawk_rtx_freevaloocstr (rtx, a0, str0);
	}

	v = hawk_rtx_makeintval (rtx, (hawk_int_t)n);
	if (v == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, v);
	return 0;
}

static int index_or_rindex (hawk_rtx_t* rtx, int rindex)
{
	/* this is similar to the built-in index() function but doesn't
	 * care about IGNORECASE. */
	hawk_oow_t nargs;
	hawk_val_t* a0, * a1;
	hawk_int_t idx, boundary = 1;

	nargs = hawk_rtx_getnargs(rtx);
	a0 = hawk_rtx_getarg(rtx, 0);
	a1 = hawk_rtx_getarg(rtx, 1);

	/*
	index ("abc", "d", 3);
	rindex ("abcdefabcdx", "cd", 8);
	*/

	if (nargs >= 3) 
	{
		hawk_val_t* a2;
		int n;

		a2 = hawk_rtx_getarg(rtx, 2);
		n = hawk_rtx_valtoint(rtx, a2, &boundary);
		if (n <= -1) return -1;
	}

	if (HAWK_RTX_GETVALTYPE(rtx, a0) == HAWK_VAL_MBS)
	{
		hawk_bch_t* str0, * str1, * ptr;
		hawk_oow_t len0, len1;

		str0 = ((hawk_val_mbs_t*)a0)->val.ptr;
		len0 = ((hawk_val_mbs_t*)a0)->val.len;

		str1 = hawk_rtx_getvalbcstr(rtx, a1, &len1);
		if (!str1) return -1;

		if (nargs < 3)
		{
			boundary = rindex? len0: 1;
		}
		else
		{
			if (boundary == 0) boundary = 1;
			else if (boundary < 0) boundary = len0 + boundary + 1;
		}

		if (boundary > len0 || boundary <= 0)
		{
			ptr = HAWK_NULL;
		}
		else if (rindex)
		{
			/* 'boundary' acts as an end position */
			ptr = hawk_rfind_bchars_in_bchars(&str0[0], boundary, str1, len1, rtx->gbl.ignorecase);
		}
		else
		{
			/* 'boundary' acts as an start position */
			ptr = hawk_find_bchars_in_bchars(&str0[boundary-1], len0 - boundary + 1, str1, len1, rtx->gbl.ignorecase);
		}

		idx = (ptr == HAWK_NULL)? 0: ((hawk_int_t)(ptr - str0) + 1);

		hawk_rtx_freevalbcstr (rtx, a1, str1);
	}
	else
	{
		hawk_ooch_t* str0, * str1, * ptr;
		hawk_oow_t len0, len1;

		str0 = hawk_rtx_getvaloocstr(rtx, a0, &len0);
		if (!str0) return -1;

		str1 = hawk_rtx_getvaloocstr(rtx, a1, &len1);
		if (!str1)
		{
			hawk_rtx_freevaloocstr (rtx, a0, str0);
			return -1;
		}

		if (nargs < 3)
		{
			boundary = rindex? len0: 1;
		}
		else
		{
			if (boundary == 0) boundary = 1;
			else if (boundary < 0) boundary = len0 + boundary + 1;
		}

		if (boundary > len0 || boundary <= 0)
		{
			ptr = HAWK_NULL;
		}
		else if (rindex)
		{
			/* 'boundary' acts as an end position */
			ptr = hawk_rfind_oochars_in_oochars(&str0[0], boundary, str1, len1, rtx->gbl.ignorecase);
		}
		else
		{
			/* 'boundary' acts as an start position */
			ptr = hawk_find_oochars_in_oochars(&str0[boundary-1], len0 - boundary + 1, str1, len1, rtx->gbl.ignorecase);
		}

		idx = (ptr == HAWK_NULL)? 0: ((hawk_int_t)(ptr - str0) + 1);

		hawk_rtx_freevaloocstr (rtx, a1, str1);
		hawk_rtx_freevaloocstr (rtx, a0, str0);
	}

	a0 = hawk_rtx_makeintval(rtx, idx);
	if (a0 == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, a0);
	return 0;
}

int hawk_fnc_index (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	return index_or_rindex (rtx, 0);
}

int hawk_fnc_rindex (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	return index_or_rindex (rtx, 1);
}

int hawk_fnc_length (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_oow_t nargs;
	hawk_val_t* v;
	hawk_val_type_t vtype;
	hawk_ooch_t* str;
	hawk_oow_t len;

	nargs = hawk_rtx_getnargs (rtx);
	HAWK_ASSERT (nargs >= 0 && nargs <= 1);
	
	if (nargs == 0)
	{
		/* get the length of $0 */
		len = HAWK_OOECS_LEN(&rtx->inrec.line);
	}
	else
	{
		v = hawk_rtx_getarg (rtx, 0);
		vtype = HAWK_RTX_GETVALTYPE (rtx, v);

		switch (vtype)
		{
			case HAWK_VAL_MAP:
				/* map size */
				len = HAWK_HTB_SIZE(((hawk_val_map_t*)v)->map);
				break;

			case HAWK_VAL_STR:
				/* string length */
				len = ((hawk_val_str_t*)v)->val.len;
				break;

			case HAWK_VAL_MBS:
				len = ((hawk_val_mbs_t*)v)->val.len;
				break;

			default:
				/* convert to string and get length */
				str = hawk_rtx_valtooocstrdup(rtx, v, &len);
				if (!str) return -1;
				hawk_rtx_freemem (rtx, str);
		}
	}

	v = hawk_rtx_makeintval(rtx, len);
	if (!v) return -1;

	hawk_rtx_setretval (rtx, v);
	return 0;
}

int hawk_fnc_substr (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_oow_t nargs;
	hawk_val_t* a0, * a1, * a2, * r;
	hawk_int_t lindex, lcount;
	int n;

	nargs = hawk_rtx_getnargs(rtx);
	HAWK_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = hawk_rtx_getarg(rtx, 0);
	a1 = hawk_rtx_getarg(rtx, 1);
	a2 = (nargs >= 3)? hawk_rtx_getarg(rtx, 2): HAWK_NULL;

	n = hawk_rtx_valtoint(rtx, a1, &lindex);
	if (n <= -1) return -1;

	if (a2)
	{
		n = hawk_rtx_valtoint(rtx, a2, &lcount);
		if (n <= -1) return -1;
		if (lcount < 0) lcount = 0;
	}
	else lcount = HAWK_TYPE_MAX(hawk_int_t);

	lindex = lindex - 1;
	if (lindex < 0) lindex = 0;

	if (HAWK_RTX_GETVALTYPE(rtx, a0) == HAWK_VAL_MBS)
	{
		hawk_bch_t* str;
		hawk_oow_t len;

		str = ((hawk_val_mbs_t*)a0)->val.ptr;
		len = ((hawk_val_mbs_t*)a0)->val.len;

		if (lindex >= (hawk_int_t)len) lindex = (hawk_int_t)len;
		if (lcount > (hawk_int_t)len - lindex) lcount = (hawk_int_t)len - lindex;

		r = hawk_rtx_makembsvalwithbchars(rtx, &str[lindex], (hawk_oow_t)lcount);
		if (!r) return -1;
	}
	else
	{
		hawk_ooch_t* str;
		hawk_oow_t len;

		str = hawk_rtx_getvaloocstr(rtx, a0, &len);
		if (!str) return -1;

		if (lindex >= (hawk_int_t)len) lindex = (hawk_int_t)len;
		if (lcount > (hawk_int_t)len - lindex) lcount = (hawk_int_t)len - lindex;

		r = hawk_rtx_makestrvalwithoochars(rtx, &str[lindex], (hawk_oow_t)lcount);
		hawk_rtx_freevaloocstr (rtx, a0, str);
		if (!r) return -1;
	}

	hawk_rtx_setretval (rtx, r);
	return 0;
}

int hawk_fnc_split (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_oow_t nargs;
	hawk_val_t* a0, * a2, * t1, * t2;
	hawk_val_type_t a2_vtype, t1_vtype;

	hawk_oocs_t str;
	hawk_oocs_t fs;
	hawk_ooch_t* fs_free = HAWK_NULL;
	const hawk_ooch_t* p;
	hawk_oow_t str_left, org_len;
	hawk_tre_t* fs_rex = HAWK_NULL; 
	hawk_tre_t* fs_rex_free = HAWK_NULL;

	hawk_oocs_t tok;
	hawk_int_t nflds;
	int x;

	str.ptr = HAWK_NULL;
	str.len = 0;

	nargs = hawk_rtx_getnargs(rtx);
	HAWK_ASSERT (nargs >= 2 && nargs <= 3);

	a0 = hawk_rtx_getarg(rtx, 0);
	a2 = (nargs >= 3)? hawk_rtx_getarg (rtx, 2): HAWK_NULL;

	str.ptr = hawk_rtx_getvaloocstr(rtx, a0, &str.len);
	if (HAWK_UNLIKELY(!str.ptr)) goto oops;

	if (a2 == HAWK_NULL)
	{
		/* get the value from FS */
		t1 = hawk_rtx_getgbl(rtx, HAWK_GBL_FS);
		t1_vtype = HAWK_RTX_GETVALTYPE(rtx, t1);
		if (t1_vtype == HAWK_VAL_NIL)
		{
			fs.ptr = HAWK_T(" ");
			fs.len = 1;
		}
		else if (t1_vtype == HAWK_VAL_STR)
		{
			fs.ptr = ((hawk_val_str_t*)t1)->val.ptr;
			fs.len = ((hawk_val_str_t*)t1)->val.len;
		}
		else
		{
			fs.ptr = hawk_rtx_valtooocstrdup(rtx, t1, &fs.len);
			if (fs.ptr == HAWK_NULL) goto oops;
			fs_free = (hawk_ooch_t*)fs.ptr;
		}

		if (fs.len > 1) fs_rex = rtx->gbl.fs[rtx->gbl.ignorecase];
	}
	else 
	{
		a2_vtype = HAWK_RTX_GETVALTYPE (rtx, a2);

		if (a2_vtype == HAWK_VAL_REX)
		{
			/* the third parameter is a regular expression */
			fs_rex = ((hawk_val_rex_t*)a2)->code[rtx->gbl.ignorecase];

			/* make the loop below to take fs_rex by 
			 * setting fs_len greater than 1*/
			fs.ptr = HAWK_NULL;
			fs.len = 2;
		}
		else 
		{
			if (a2_vtype == HAWK_VAL_STR)
			{
				fs.ptr = ((hawk_val_str_t*)a2)->val.ptr;
				fs.len = ((hawk_val_str_t*)a2)->val.len;
			}
			else
			{
				fs.ptr = hawk_rtx_valtooocstrdup(rtx, a2, &fs.len);
				if (fs.ptr == HAWK_NULL) goto oops;
				fs_free = (hawk_ooch_t*)fs.ptr;
			}

			if (fs.len > 1) 
			{
				int x;

				x = rtx->gbl.ignorecase?
					hawk_rtx_buildrex(rtx, fs.ptr, fs.len, HAWK_NULL, &fs_rex):
					hawk_rtx_buildrex(rtx, fs.ptr, fs.len, &fs_rex, HAWK_NULL);
				if (x <= -1) goto oops;

				fs_rex_free = fs_rex;
			}
		}
	}

	t1 = hawk_rtx_makemapval(rtx);
	if (HAWK_UNLIKELY(!t1)) goto oops;

	hawk_rtx_refupval (rtx, t1);
	x = hawk_rtx_setrefval(rtx, (hawk_val_ref_t*)hawk_rtx_getarg(rtx, 1), t1);
	hawk_rtx_refdownval (rtx, t1);
	if (HAWK_UNLIKELY(x <= -1)) goto oops;

	/* fill the map with actual values */
	p = str.ptr; str_left = str.len; org_len = str.len;
	nflds = 0;

	while (p)
	{
		hawk_ooch_t key_buf[HAWK_SIZEOF(hawk_int_t)*8+2];
		hawk_oow_t key_len;

		if (fs.len <= 1)
		{
			p = hawk_rtx_strxntok(rtx, p, str.len, fs.ptr, fs.len, &tok);
		}
		else
		{
			p = hawk_rtx_strxntokbyrex(rtx, str.ptr, org_len, p, str.len, fs_rex, &tok);
			if (p == HAWK_NULL && hawk_rtx_geterrnum(rtx) != HAWK_ENOERR)
			{
				goto oops;
			}
		}

		if (nflds == 0 && p == HAWK_NULL && tok.len == 0) 
		{
			/* no field at all*/
			break; 
		}

		HAWK_ASSERT ((tok.ptr != HAWK_NULL && tok.len > 0) || tok.len == 0);

		/* create the field string - however, the split function must
		 * create a numeric value if the string is a number */
		/*t2 = hawk_rtx_makestrvalwithoocs (rtx, &tok);*/
		/*t2 = hawk_rtx_makenstrvalwithoocs(rtx, &tok); */
		t2 = hawk_rtx_makenumorstrvalwithoochars(rtx, tok.ptr, tok.len);
		if (HAWK_UNLIKELY(!t2)) goto oops;

		/* put it into the map */
		key_len = hawk_int_to_oocstr(++nflds, 10, HAWK_NULL, key_buf, HAWK_COUNTOF(key_buf));
		HAWK_ASSERT (key_len != (hawk_oow_t)-1);

		if (hawk_rtx_setmapvalfld(rtx, t1, key_buf, key_len, t2) == HAWK_NULL)
		{
			hawk_rtx_refupval (rtx, t2);
			hawk_rtx_refdownval (rtx, t2);
			goto oops;
		}

		str.len = str_left - (p - str.ptr);
	}

	/*if (str_free) hawk_rtx_freemem (rtx, str_free);*/
	hawk_rtx_freevaloocstr (rtx, a0, str.ptr);

	if (fs_free) hawk_rtx_freemem (rtx, fs_free);

	if (fs_rex_free) 
	{
		if (rtx->gbl.ignorecase)
			hawk_rtx_freerex (rtx, HAWK_NULL, fs_rex_free);
		else
			hawk_rtx_freerex (rtx, fs_rex_free, HAWK_NULL);
	}

	/*nflds--;*/

	t1 = hawk_rtx_makeintval (rtx, nflds);
	if (t1 == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, t1);
	return 0;

oops:
	if (str.ptr) hawk_rtx_freevaloocstr (rtx, a0, str.ptr);

	if (fs_free) hawk_rtx_freemem (rtx, fs_free);

	if (fs_rex_free) 
	{
		if (rtx->gbl.ignorecase)
			hawk_rtx_freerex (rtx, HAWK_NULL, fs_rex_free);
		else
			hawk_rtx_freerex (rtx, fs_rex_free, HAWK_NULL);
	}
	return -1;
}

int hawk_fnc_tolower (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_oow_t i;
	hawk_val_t* a0, * r;

	a0 = hawk_rtx_getarg (rtx, 0);
	if (HAWK_RTX_GETVALTYPE(rtx, a0) == HAWK_VAL_MBS)
	{
		hawk_bcs_t str;
		str.ptr = hawk_rtx_getvalbcstr(rtx, a0, &str.len);
		if (!str.ptr) return -1;
		r = hawk_rtx_makembsvalwithbcs(rtx, &str);
		hawk_rtx_freevalbcstr (rtx, a0, str.ptr);
		if (!r) return -1;
		str.ptr = ((hawk_val_mbs_t*)r)->val.ptr;
		str.len = ((hawk_val_mbs_t*)r)->val.len;
		for (i = 0; i < str.len; i++) str.ptr[i] = hawk_to_bch_lower(str.ptr[i]);
	}
	else
	{
		hawk_oocs_t str;
		str.ptr = hawk_rtx_getvaloocstr(rtx, a0, &str.len);
		if (!str.ptr) return -1;
		r = hawk_rtx_makestrvalwithoocs(rtx, &str);
		hawk_rtx_freevaloocstr (rtx, a0, str.ptr);
		if (!r) return -1;
		str.ptr = ((hawk_val_str_t*)r)->val.ptr;
		str.len = ((hawk_val_str_t*)r)->val.len;
		for (i = 0; i < str.len; i++) str.ptr[i] = hawk_to_ooch_lower(str.ptr[i]);
	}
	hawk_rtx_setretval (rtx, r);
	return 0;
}

int hawk_fnc_toupper (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_oow_t i;
	hawk_val_t* a0, * r;

	a0 = hawk_rtx_getarg (rtx, 0);
	if (HAWK_RTX_GETVALTYPE(rtx, a0) == HAWK_VAL_MBS)
	{
		hawk_bcs_t str;
		str.ptr = hawk_rtx_getvalbcstr(rtx, a0, &str.len);
		if (!str.ptr) return -1;
		r = hawk_rtx_makembsvalwithbcs(rtx, &str);
		hawk_rtx_freevalbcstr (rtx, a0, str.ptr);
		if (!r) return -1;
		str.ptr = ((hawk_val_mbs_t*)r)->val.ptr;
		str.len = ((hawk_val_mbs_t*)r)->val.len;
		for (i = 0; i < str.len; i++) str.ptr[i] = hawk_to_bch_upper(str.ptr[i]);
	}
	else
	{
		hawk_oocs_t str;
		str.ptr = hawk_rtx_getvaloocstr(rtx, a0, &str.len);
		if (!str.ptr) return -1;
		r = hawk_rtx_makestrvalwithoocs(rtx, &str);
		hawk_rtx_freevaloocstr (rtx, a0, str.ptr);
		if (!r) return -1;
		str.ptr = ((hawk_val_str_t*)r)->val.ptr;
		str.len = ((hawk_val_str_t*)r)->val.len;
		for (i = 0; i < str.len; i++) str.ptr[i] = hawk_to_ooch_upper(str.ptr[i]);
	}
	hawk_rtx_setretval (rtx, r);
	return 0;
}

static int __substitute_oocs (hawk_rtx_t* rtx, hawk_oow_t* max_count, hawk_tre_t* rex, hawk_oocs_t* s1, hawk_oocs_t* s2, hawk_ooecs_t* new)
{
	hawk_oocs_t mat, pmat, cur;
	hawk_oow_t sub_count, match_limit;
	hawk_ooch_t* s2_end;

	s2_end = s2->ptr + s2->len;
	cur.ptr = s2->ptr;
	cur.len = s2->len;
	sub_count = 0;
	match_limit = *max_count;

	pmat.ptr = HAWK_NULL;
	pmat.len = 0;

	/* perform test when cur_ptr == s2_end also because
	 * end of string($) needs to be tested */
	while (cur.ptr <= s2_end)
	{
		int n;
		hawk_oow_t m, i;

		if (sub_count < match_limit)
		{
			n = hawk_rtx_matchrexwithoocs(rtx, rex, s2, &cur, &mat, HAWK_NULL);
			if (HAWK_UNLIKELY(n <= -1)) goto oops;
		}
		else n = 0;

		if (n == 0) 
		{ 
			/* no more match found */
			if (hawk_ooecs_ncat(new, cur.ptr, cur.len) == (hawk_oow_t)-1) goto oops;
			break;
		}

		if (mat.len == 0 && pmat.ptr != HAWK_NULL && mat.ptr == pmat.ptr + pmat.len)
		{
			/* match length is 0 and the match is still at the
			 * end of the previous match */
			goto skip_one_char;
		}

		if (hawk_ooecs_ncat(new, cur.ptr, mat.ptr - cur.ptr) == (hawk_oow_t)-1) goto oops;

		for (i = 0; i < s1->len; i++)
		{
			if ((i+1) < s1->len && s1->ptr[i] == '\\' && s1->ptr[i+1] == '&')
			{
				m = hawk_ooecs_ccat(new, '&');
				i++;
			}
			else if (s1->ptr[i] == '&')
			{
				m = hawk_ooecs_ncat(new, mat.ptr, mat.len);
			}
			else 
			{
				m = hawk_ooecs_ccat(new, s1->ptr[i]);
			}

			if (HAWK_UNLIKELY(m == (hawk_oow_t)-1)) goto oops;
		}

		sub_count++;
		cur.len = cur.len - ((mat.ptr - cur.ptr) + mat.len);
		cur.ptr = mat.ptr + mat.len;

		pmat = mat;

		if (mat.len == 0)
		{
		skip_one_char:
			/* special treatment is needed if match length is 0 */
			if (cur.ptr < s2_end) /* $ matches at s2_end. with this check, '\0' or whatever character after the end may get appended redundantly */
			{
				m = hawk_ooecs_ncat(new, cur.ptr, 1);
				if (HAWK_UNLIKELY(m == (hawk_oow_t)-1)) goto oops;
			}
			cur.ptr++; cur.len--;
		}
	}

	*max_count = sub_count;
	return 0;

oops:
	return -1;
}

static int __substitute_bcs (hawk_rtx_t* rtx, hawk_oow_t* max_count, hawk_tre_t* rex, hawk_bcs_t* s1, hawk_bcs_t* s2, hawk_becs_t* new)
{
	hawk_bcs_t mat, pmat, cur;
	hawk_oow_t sub_count, match_limit;
	hawk_bch_t* s2_end;

	s2_end = s2->ptr + s2->len;
	cur.ptr = s2->ptr;
	cur.len = s2->len;
	sub_count = 0;
	match_limit = *max_count;

	pmat.ptr = HAWK_NULL;
	pmat.len = 0;

	/* perform test when cur_ptr == s2_end also because
	 * end of string($) needs to be tested */
	while (cur.ptr <= s2_end)
	{
		int n;
		hawk_oow_t m, i;

		if (sub_count < match_limit)
		{
			n = hawk_rtx_matchrexwithbcs(rtx, rex, s2, &cur, &mat, HAWK_NULL);
			if (HAWK_UNLIKELY(n <= -1)) goto oops;
		}
		else n = 0;

		if (n == 0) 
		{ 
			/* no more match found */
			if (hawk_becs_ncat(new, cur.ptr, cur.len) == (hawk_oow_t)-1) goto oops;
			break;
		}

		if (mat.len == 0 && pmat.ptr != HAWK_NULL && mat.ptr == pmat.ptr + pmat.len)
		{
			/* match length is 0 and the match is still at the
			 * end of the previous match */
			goto skip_one_char;
		}

		if (hawk_becs_ncat(new, cur.ptr, mat.ptr - cur.ptr) == (hawk_oow_t)-1) goto oops;

		for (i = 0; i < s1->len; i++)
		{
			if ((i+1) < s1->len && s1->ptr[i] == '\\' && s1->ptr[i+1] == '&')
			{
				m = hawk_becs_ccat(new, '&');
				i++;
			}
			else if (s1->ptr[i] == '&')
			{
				m = hawk_becs_ncat(new, mat.ptr, mat.len);
			}
			else 
			{
				m = hawk_becs_ccat(new, s1->ptr[i]);
			}

			if (HAWK_UNLIKELY(m == (hawk_oow_t)-1)) goto oops;
		}

		sub_count++;
		cur.len = cur.len - ((mat.ptr - cur.ptr) + mat.len);
		cur.ptr = mat.ptr + mat.len;

		pmat = mat;

		if (mat.len == 0)
		{
		skip_one_char:
			/* special treatment is needed if match length is 0 */
			if (cur.ptr < s2_end) /* $ matches at s2_end. with this check, '\0' or whatever character after the end may get appended redundantly */
			{
				m = hawk_becs_ncat(new, cur.ptr, 1);
				if (HAWK_UNLIKELY(m == (hawk_oow_t)-1)) goto oops;
			}
			cur.ptr++; cur.len--;
		}
	}

	*max_count = sub_count;
	return 0;

oops:
	return -1;
}

static int __substitute (hawk_rtx_t* rtx, hawk_oow_t max_count)
{
	hawk_oow_t nargs;
	hawk_val_t* a0, * a1, * r2, * v;
	hawk_val_type_t a0_vtype;

	hawk_oocs_t s0;
	hawk_ptl_t s1;
	hawk_ptl_t s2;

	int s1_free = 0;
	int s2_free = 0;

	hawk_tre_t* rex = HAWK_NULL;
	hawk_tre_t* rex_free = HAWK_NULL;
	hawk_oow_t sub_count;

	s0.ptr = HAWK_NULL;
	s0.len = 0;

	s1.ptr = HAWK_NULL;
	s1.len = 0;

	nargs = hawk_rtx_getnargs(rtx);
	a0 = hawk_rtx_getarg(rtx, 0);
	a1 = hawk_rtx_getarg(rtx, 1);

	a0_vtype = HAWK_RTX_GETVALTYPE(rtx, a0);

	/* the first argument - pattern */
	if (a0_vtype == HAWK_VAL_REX)
	{
		rex = ((hawk_val_rex_t*)a0)->code[rtx->gbl.ignorecase];
	}
	else
	{
		s0.ptr = hawk_rtx_getvaloocstr(rtx, a0, &s0.len);
		if (HAWK_UNLIKELY(!s0.ptr)) goto oops;
	}

	/* the optional third argument - string to manipulate */
	if (nargs < 3)
	{
		/* is this correct? any needs to use inrec.d0? */
		s2.ptr = HAWK_OOECS_PTR(&rtx->inrec.line);
		s2.len = HAWK_OOECS_LEN(&rtx->inrec.line);

		/* the second argument - substitute */
		s1.ptr = hawk_rtx_getvaloocstr(rtx, a1, &s1.len);
		s1_free = 1;
	}
	else 
	{
		r2 = hawk_rtx_getrefval(rtx, (hawk_val_ref_t*)hawk_rtx_getarg(rtx, 2));

		if (HAWK_RTX_GETVALTYPE(rtx, r2) == HAWK_VAL_MBS)
		{
			s2.ptr = hawk_rtx_getvalbcstr(rtx, r2, &s2.len);
			s2_free = 2;

			/* the second argument - substitute */
			s1.ptr = hawk_rtx_getvalbcstr(rtx, a1, &s1.len);
			s1_free = 2;
		}
		else
		{
			s2.ptr = hawk_rtx_getvaloocstr(rtx, r2, &s2.len);
			s2_free = 1;

			/* the second argument - substitute */
			s1.ptr = hawk_rtx_getvaloocstr(rtx, a1, &s1.len);
			s1_free = 1;
		}
	}

	if (HAWK_UNLIKELY(!s1.ptr || !s2.ptr)) goto oops;

	if (a0_vtype != HAWK_VAL_REX)
	{
		int x;

		x = rtx->gbl.ignorecase?
			hawk_rtx_buildrex(rtx, s0.ptr, s0.len, HAWK_NULL, &rex):
			hawk_rtx_buildrex(rtx, s0.ptr, s0.len, &rex, HAWK_NULL);
		if (HAWK_UNLIKELY(x <= -1)) goto oops;

		rex_free = rex;
	}

	sub_count = max_count;
	if (s2_free == 2)
	{
		hawk_becs_clear(&rtx->fnc.bout);
		if (__substitute_bcs(rtx, &sub_count, rex, (hawk_bcs_t*)&s1, (hawk_bcs_t*)&s2, &rtx->fnc.bout) <= -1) goto oops;
	}
	else
	{
		hawk_ooecs_clear(&rtx->fnc.oout);
		if (__substitute_oocs(rtx, &sub_count, rex, (hawk_oocs_t*)&s1, (hawk_oocs_t*)&s2, &rtx->fnc.oout) <= -1) goto oops;
	}

	if (rex_free)
	{
		if (rtx->gbl.ignorecase)
			hawk_rtx_freerex (rtx, HAWK_NULL, rex_free); 
		else
			hawk_rtx_freerex (rtx, rex_free, HAWK_NULL); 
		rex_free = HAWK_NULL;
	}

	switch (s2_free)
	{
		case 1:
			hawk_rtx_freevaloocstr (rtx, r2, s2.ptr);
			break;
		case 2:
			hawk_rtx_freevalbcstr (rtx, r2, s2.ptr);
			break;
	}
	s2.ptr = HAWK_NULL;

	switch (s1_free)
	{
		case 1:
			hawk_rtx_freevaloocstr (rtx, a1, s1.ptr);
			break;
		case 2:
			hawk_rtx_freevalbcstr (rtx, a1, s1.ptr);
			break;
	}
	s1.ptr = HAWK_NULL;

	if (s0.ptr) 
	{
		hawk_rtx_freevaloocstr (rtx, a0, s0.ptr);
		s0.ptr = HAWK_NULL;
	}

	if (sub_count > 0)
	{
		int n;
		if (nargs < 3)
		{
			n = hawk_rtx_setrec(rtx, 0, HAWK_OOECS_OOCS(&rtx->fnc.oout), 0);
		}
		else
		{
			v = (s2_free == 2)?
				hawk_rtx_makembsvalwithbcs(rtx, HAWK_BECS_BCS(&rtx->fnc.bout)):
				hawk_rtx_makestrvalwithoocs(rtx, HAWK_OOECS_OOCS(&rtx->fnc.oout));
			if (HAWK_UNLIKELY(!v)) goto oops;

			hawk_rtx_refupval (rtx, v);
			n = hawk_rtx_setrefval(rtx, (hawk_val_ref_t*)hawk_rtx_getarg(rtx, 2), v);
			hawk_rtx_refdownval (rtx, v);
		}
		if (HAWK_UNLIKELY(n <= -1)) goto oops;
	}

	v = hawk_rtx_makeintval(rtx, sub_count);
	if (HAWK_UNLIKELY(!v)) return -1;

	hawk_rtx_setretval (rtx, v);
	return 0;

oops:
	if (rex_free) 
	{
		if (rtx->gbl.ignorecase)
			hawk_rtx_freerex (rtx, HAWK_NULL, rex_free);
		else	
			hawk_rtx_freerex (rtx, rex_free, HAWK_NULL); 
	}

	if (s2.ptr)
	{
		switch (s2_free)
		{
			case 1:
				hawk_rtx_freevaloocstr (rtx, r2, s2.ptr);
				break;
			case 2:
				hawk_rtx_freevalbcstr (rtx, r2, s2.ptr);
				break;
		}
	}

	if (s1.ptr)
	{
		switch (s1_free)
		{
			case 1:
				hawk_rtx_freevaloocstr (rtx, a1, s1.ptr);
				break;
			case 2:
				hawk_rtx_freevalbcstr (rtx, a1, s1.ptr);
				break;
		}
	}

	if (s0.ptr) hawk_rtx_freevaloocstr (rtx, a0, s0.ptr);
	return -1;
}

int hawk_fnc_gsub (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	/* gsub(/a/, "#");
	 * x="abac"; gsub(/a/, "#", x); */
	return __substitute(rtx, HAWK_TYPE_MAX(hawk_oow_t));
}

int hawk_fnc_sub (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	/* sub(/a/, "#");
	 * x="abac"; sub(/a/, "#", x); */
	return __substitute(rtx, 1);
}

int hawk_fnc_match (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	/* 
	match("abcdefg", "cde");
	match("abcdefgdefx", "def", 7);
	------------------------------------
	match("ab\uB098cdefgdefx", /(def)g(.+)/, 1, x); 
	q = length(x) / 2;
	for (i = 1; i <= q; i++) print x[i,"start"], x[i,"length"]; 
	print RSTART, RLENGTH;
	* ------------------------------------
	match(B"ab\xB0\x98cdefgdefx", /(def)g(.+)/, 1, x); 
	q = length(x) / 2;
	for (i = 1; i <= q; i++) print x[i,"start"], x[i,"length"]; 
	print RSTART, RLENGTH;
	*/

	hawk_oow_t nargs;
	hawk_val_t* a0, * a1;
	hawk_val_type_t a0_type;

	union
	{
		hawk_ooch_t* o;
		hawk_bch_t* b;
	} str0;
	hawk_oow_t len0;
	hawk_int_t idx, start = 1;
	hawk_val_t* x0 = HAWK_NULL, * x1 = HAWK_NULL, * x2 = HAWK_NULL;

	union
	{
		hawk_oocs_t o;
		hawk_bcs_t b;
	} mat;

	union
	{
		hawk_oocs_t o[9];
		hawk_bcs_t b[9];
	} submat;

	int n;

	nargs = hawk_rtx_getnargs(rtx);
	HAWK_ASSERT (nargs >= 2 && nargs <= 4);
	
	a0 = hawk_rtx_getarg(rtx, 0);
	a1 = hawk_rtx_getarg(rtx, 1);

	if (nargs >= 3) 
	{
		hawk_val_t* a2;

		a2 = hawk_rtx_getarg(rtx, 2);
		/* if the 3rd parameter is not an array,
		 * it is treated as a match start index */
		n = hawk_rtx_valtoint(rtx, a2, &start);
		if (n <= -1) return -1;
	}

	HAWK_MEMSET (&submat, 0, HAWK_SIZEOF(submat));

	a0_type = HAWK_RTX_GETVALTYPE(rtx, a0);
	if (a0_type == HAWK_VAL_MBS)
	{
		hawk_bcs_t tmp;

		str0.b = hawk_rtx_getvalbcstr(rtx, a0, &len0);
		if (HAWK_UNLIKELY(!str0.b)) return -1;

		if (start == 0) start = 1;
		else if (start < 0) start = len0 + start + 1;
		if (start > len0 || start <= 0) n = 0;

		tmp.ptr = str0.b + start - 1;
		tmp.len = len0 - start + 1;

		n = hawk_rtx_matchvalwithbcs(rtx, a1, &tmp, &tmp, &mat.b, (nargs >= 4? submat.b: HAWK_NULL));
		hawk_rtx_freevalbcstr (rtx, a0, str0.b);

		if (n <= -1) return -1;

		/* RSTART: 0 on no match */
		idx = (n == 0)? 0: ((hawk_int_t)(mat.b.ptr - str0.b) + 1);
	}
	else
	{
		hawk_oocs_t tmp;

		str0.o = hawk_rtx_getvaloocstr(rtx, a0, &len0);
		if (HAWK_UNLIKELY(!str0.o)) return -1;

		if (start == 0) start = 1;
		else if (start < 0) start = len0 + start + 1;
		if (start > len0 || start <= 0) n = 0;

		tmp.ptr = str0.o + start - 1;
		tmp.len = len0 - start + 1;

		n = hawk_rtx_matchvalwithoocs(rtx, a1, &tmp, &tmp, &mat.o, (nargs >= 4? submat.o: HAWK_NULL));
		hawk_rtx_freevaloocstr (rtx, a0, str0.o);

		if (n <= -1) return -1;

		/* RSTART: 0 on no match */
		idx = (n == 0)? 0: ((hawk_int_t)(mat.o.ptr - str0.o) + 1);
	}

	x0 = hawk_rtx_makeintval(rtx, idx);
	if (!x0) goto oops;
	hawk_rtx_refupval (rtx, x0);

	/* RLENGTH: -1 on no match */
	HAWK_ASSERT (&mat.o.len == &mat.b.len);
	x1 = hawk_rtx_makeintval(rtx, ((n == 0)? (hawk_int_t)-1: (hawk_int_t)mat.o.len)); /* just use mat.o.len regardless of a0_type */
	if (!x1) goto oops;
	hawk_rtx_refupval (rtx, x1);

	if (nargs >= 4)
	{
		const hawk_oocs_t* subsep;
		hawk_int_t submatcount;
		hawk_oow_t i, xlen;
		hawk_val_t* tv;

		hawk_ooecs_clear (&rtx->fnc.oout);

		x2 = hawk_rtx_makemapval(rtx);
		if (HAWK_UNLIKELY(!x2)) goto oops;
		hawk_rtx_refupval (rtx, x2);

		submatcount = 0;
		subsep = hawk_rtx_getsubsep(rtx);
		for (i = 0; i < HAWK_COUNTOF(submat.o); i++)
		{
			HAWK_ASSERT ((void*)&submat.o[i] == (void*)&submat.b[i]);
			if (!submat.o[i].ptr) break;

			submatcount++;

			if (hawk_ooecs_fmt(&rtx->fnc.oout, HAWK_T("%d"), (int)submatcount) == (hawk_oow_t)-1 ||
			    hawk_ooecs_ncat(&rtx->fnc.oout, subsep->ptr, subsep->len) == (hawk_oow_t)-1) goto oops;
			xlen = HAWK_OOECS_LEN(&rtx->fnc.oout);
			if (hawk_ooecs_ncat(&rtx->fnc.oout, HAWK_T("start"), 5) == (hawk_oow_t)-1) goto oops;

			tv = hawk_rtx_makeintval(rtx, ((a0_type == HAWK_VAL_MBS)? submat.b[i].ptr - str0.b + 1: submat.o[i].ptr - str0.o + 1));
			if (!tv) goto oops;
			if (!hawk_rtx_setmapvalfld(rtx, x2, HAWK_OOECS_PTR(&rtx->fnc.oout), HAWK_OOECS_LEN(&rtx->fnc.oout), tv))
			{
				hawk_rtx_refupval (rtx, tv);
				hawk_rtx_refdownval (rtx, tv);
				goto oops;
			}

			if (hawk_ooecs_setlen(&rtx->fnc.oout, xlen) == (hawk_oow_t)-1 ||
			    hawk_ooecs_ncat(&rtx->fnc.oout, HAWK_T("length"), 6) == (hawk_oow_t)-1) goto oops;

			tv = hawk_rtx_makeintval(rtx, submat.o[i].len);
			if (!tv) goto oops;
			if (!hawk_rtx_setmapvalfld(rtx, x2, HAWK_OOECS_PTR(&rtx->fnc.oout), HAWK_OOECS_LEN(&rtx->fnc.oout), tv))
			{
				hawk_rtx_refupval (rtx, tv);
				hawk_rtx_refdownval (rtx, tv);
				goto oops;
			}
		}
		/* the caller of this function must be able to get the submatch count by
		 * dividing the array size by 2 */

		if (hawk_rtx_setrefval(rtx, (hawk_val_ref_t*)hawk_rtx_getarg(rtx, 3), x2) <= -1) goto oops;
	}

	if (hawk_rtx_setgbl(rtx, HAWK_GBL_RSTART, x0) <= -1 ||
	    hawk_rtx_setgbl(rtx, HAWK_GBL_RLENGTH, x1) <= -1)
	{
		goto oops;
	}

	hawk_rtx_setretval (rtx, x0);

	if (x2) hawk_rtx_refdownval (rtx, x2);
	hawk_rtx_refdownval (rtx, x1);
	hawk_rtx_refdownval (rtx, x0);
	return 0;

oops:
	if (x2) hawk_rtx_refdownval (rtx, x2);
	if (x1) hawk_rtx_refdownval (rtx, x1);
	if (x0) hawk_rtx_refdownval (rtx, x0);
	return -1;
}

int hawk_fnc_sprintf (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_oow_t nargs;
	hawk_val_t* a0;

	nargs = hawk_rtx_getnargs(rtx);
	HAWK_ASSERT (nargs > 0);

	a0 = hawk_rtx_getarg(rtx, 0);
	if (HAWK_RTX_GETVALTYPE(rtx, a0) == HAWK_VAL_MBS)
	{
		hawk_becs_t fbu;
		int fbu_inited = 0;
		hawk_bcs_t cs0;
		hawk_bcs_t x;

		if (hawk_becs_init(&fbu, hawk_rtx_getgem(rtx), 256) <= -1) goto oops_mbs;
		fbu_inited = 1;

		cs0.ptr = hawk_rtx_getvalbcstr(rtx, a0, &cs0.len);
		if (HAWK_UNLIKELY(!cs0.ptr)) goto oops_mbs;

		x.ptr = hawk_rtx_formatmbs(rtx, &rtx->fnc.bout, &fbu, cs0.ptr, cs0.len, nargs, HAWK_NULL, &x.len);
		hawk_rtx_freevalbcstr (rtx, a0, cs0.ptr);
		if (HAWK_UNLIKELY(!x.ptr)) goto oops_mbs;
		
		a0 = hawk_rtx_makembsvalwithbcs(rtx, &x);
		if (HAWK_UNLIKELY(!a0)) goto oops_mbs;

		hawk_becs_fini (&fbu);
		hawk_rtx_setretval (rtx, a0);
		return 0;

	oops_mbs:
		if (fbu_inited) hawk_becs_fini (&fbu);
		return -1;
	}
	else
	{
		hawk_ooecs_t fbu;
		int fbu_inited = 0;
		hawk_oocs_t cs0;
		hawk_oocs_t x;

		if (hawk_ooecs_init(&fbu, hawk_rtx_getgem(rtx), 256) <= -1) goto oops;
		fbu_inited = 1;

		cs0.ptr = hawk_rtx_getvaloocstr(rtx, a0, &cs0.len);
		if (HAWK_UNLIKELY(!cs0.ptr)) goto oops;

		x.ptr = hawk_rtx_format(rtx, &rtx->fnc.oout, &fbu, cs0.ptr, cs0.len, nargs, HAWK_NULL, &x.len);
		hawk_rtx_freevaloocstr (rtx, a0, cs0.ptr);
		if (HAWK_UNLIKELY(!x.ptr)) goto oops;

		a0 = hawk_rtx_makestrvalwithoocs(rtx, &x);
		if (HAWK_UNLIKELY(!a0)) goto oops;

		hawk_ooecs_fini (&fbu);
		hawk_rtx_setretval (rtx, a0);
		return 0;

	oops:
		if (fbu_inited) hawk_ooecs_fini (&fbu);
		return -1;
	}
}

static int fnc_int (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_val_t* a0;
	hawk_int_t lv;
	hawk_val_t* r;
	int n;

	a0 = hawk_rtx_getarg(rtx, 0);

	n = hawk_rtx_valtoint(rtx, a0, &lv);
	if (n <= -1) return -1;

	r = hawk_rtx_makeintval(rtx, lv);
	if (r == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_typename (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_val_t* a0;
	hawk_val_t* r;
	const hawk_ooch_t* name;

	a0 = hawk_rtx_getarg(rtx, 0);
	name = hawk_rtx_getvaltypename(rtx, a0);

	r = hawk_rtx_makestrvalwithoocstr(rtx, name);
	if (r == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_gcrefs (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_val_t* a0;
	a0 = hawk_rtx_getarg(rtx, 0);
	hawk_rtx_setretval (rtx, hawk_rtx_makeintval(rtx, HAWK_VTR_IS_POINTER(a0)? a0->v_refs: 0));
	return 0;
}


static int fnc_isnil (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_val_t* a0;
	hawk_val_t* r;

	a0 = hawk_rtx_getarg(rtx, 0);

	r = hawk_rtx_makeintval(rtx, HAWK_RTX_GETVALTYPE(rtx, a0) == HAWK_VAL_NIL);
	if (r == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_ismap (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_val_t* a0;
	hawk_val_t* r;

	a0 = hawk_rtx_getarg(rtx, 0);

	r = hawk_rtx_makeintval(rtx, HAWK_RTX_GETVALTYPE(rtx, a0) == HAWK_VAL_MAP);
	if (r == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, r);
	return 0;
}

static HAWK_INLINE int asort_compare (const void* x1, const void* x2, void* ctx, int* cv)
{
	int n;
	if (hawk_rtx_cmpval((hawk_rtx_t*)ctx, *(hawk_val_t**)x1, *(hawk_val_t**)x2, &n) <= -1) return -1;
	*cv = n;
	return 0;
}

struct cud_t
{
	hawk_rtx_t* rtx;
	hawk_fun_t* fun;
};

static HAWK_INLINE int asort_compare_ud (const void* x1, const void* x2, void* ctx, int* cv)
{
	struct cud_t* cud = (struct cud_t*)ctx;
	hawk_val_t* r, * args[2];
	hawk_int_t rv;

	args[0] = *(hawk_val_t**)x1;
	args[1] = *(hawk_val_t**)x2;
	r = hawk_rtx_callfun(cud->rtx, cud->fun, args, 2); 
	if (!r) return -1;
	if (hawk_rtx_valtoint(cud->rtx, r,  &rv) <= -1) return -1;
	*cv = rv;
	return 0;
}

static HAWK_INLINE int __fnc_asort (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi, int sort_keys)
{
	/*
	 a["tt"]=20; a[2]=10; a[3]=99;
	 asort(a);
	 for (i in a) print i, a[i];
	 ---------------------------------------------
	 a["tt"]=20; a[2]=10; a[3]=99;
	 asort(a, b);
	 for (i in b) print i, b[i];
	 ----------------------------------------------
	 function cmp(a, b) { return b - a;  }
	 BEGIN {
	   a["tt"]=20; a[2]=10; a[3]=99; a[4]=66;
	   asort(a, b, cmp);
	   for (i in b) print i, b[i];
	 }
	 */

	hawk_oow_t nargs;
	hawk_val_t* a0, * a0_val;
	hawk_val_type_t a0_type, v_type;
	hawk_val_t* r, * rmap = HAWK_NULL;
	hawk_int_t rv = 0; /* as if no element in the map */
	hawk_val_map_itr_t itr;
	hawk_fun_t* fun = HAWK_NULL;
	hawk_oow_t msz, i;
	hawk_val_t** va;
	int x;

	nargs = hawk_rtx_getnargs(rtx);

	a0 = hawk_rtx_getarg(rtx, 0);
	a0_type = HAWK_RTX_GETVALTYPE(rtx, a0);
	HAWK_ASSERT (a0_type == HAWK_VAL_REF);

	v_type = hawk_rtx_getrefvaltype(rtx, (hawk_val_ref_t*)a0);
	if (v_type != HAWK_VAL_MAP)
	{
		if (v_type == HAWK_VAL_NIL)
		{
			/* treat it as an empty value */
			goto done;
		}

		hawk_rtx_seterrfmt (rtx, HAWK_NULL, HAWK_ENOTMAP, HAWK_T("source not a map"));
		return -1;
	}

	a0_val = hawk_rtx_getrefval(rtx, (hawk_val_ref_t*)a0);
	HAWK_ASSERT (HAWK_RTX_GETVALTYPE(rtx, a0_val) == HAWK_VAL_MAP);

	if (nargs >= 3)
	{
		hawk_val_t* a2;

		a2 = hawk_rtx_getarg(rtx, 2);
		if (HAWK_RTX_GETVALTYPE(rtx, a2) != HAWK_VAL_FUN)
		{
			hawk_rtx_seterrfmt (rtx, HAWK_NULL, HAWK_EINVAL, HAWK_T("comparator not a function"));
			return -1;
		}

		fun = ((hawk_val_fun_t*)a2)->fun;
		if (fun->nargs < 2) 
		{
			/* the comparison accepts less than 2 arguments */
			hawk_rtx_seterrfmt (rtx, HAWK_NULL, HAWK_EINVAL, HAWK_T("%.*js not accepting 2 arguments"), fun->name.len, fun->name.ptr);
			return -1;
		}
	}

	if (!hawk_rtx_getfirstmapvalitr(rtx, a0_val, &itr)) goto done; /* map empty */

	msz = hawk_map_getsize(((hawk_val_map_t*)a0_val)->map);
	HAWK_ASSERT (msz > 0);

	va = (hawk_val_t**)hawk_rtx_allocmem(rtx, msz * HAWK_SIZEOF(*va));
	if (HAWK_UNLIKELY(!va)) return -1;
	i = 0;
	if (sort_keys)
	{
		do
		{
			const hawk_oocs_t* key = HAWK_VAL_MAP_ITR_KEY(&itr);
			va[i] = hawk_rtx_makestrvalwithoocs(rtx, key);
			if (HAWK_UNLIKELY(!va[i])) 
			{
				while (i > 0)
				{
					--i;
					hawk_rtx_refdownval (rtx, va[i]);
				}
				hawk_rtx_freemem (rtx, va);
				return -1;
			}
			hawk_rtx_refupval (rtx, va[i]);
			i++;
		}
		while (hawk_rtx_getnextmapvalitr(rtx, a0_val, &itr));
	}
	else
	{
		do
		{
			va[i] = (hawk_val_t*)HAWK_VAL_MAP_ITR_VAL(&itr);
			hawk_rtx_refupval (rtx, va[i]);
			i++;
		}
		while (hawk_rtx_getnextmapvalitr(rtx, a0_val, &itr));
	}

	if (fun)
	{
		struct cud_t cud;
		cud.rtx = rtx;
		cud.fun = fun;
		x = hawk_qsortx(va, msz, HAWK_SIZEOF(*va), asort_compare_ud, &cud);
	}
	else
	{
		x = hawk_qsortx(va, msz, HAWK_SIZEOF(*va), asort_compare, rtx);
	}

	if (x <= -1 || !(rmap = hawk_rtx_makemapval(rtx)))
	{
		for (i = 0; i < msz; i++) hawk_rtx_refdownval (rtx, va[i]);
		hawk_rtx_freemem (rtx, va);
		return -1;
	}

	for (i = 0; i < msz; i++)
	{
		hawk_ooch_t ridx[128]; /* TODO: make it dynamic? can overflow? */
		hawk_oow_t ridx_len;

		ridx_len = hawk_fmt_uintmax_to_oocstr(
			ridx,
			HAWK_COUNTOF(ridx),
			i,
			10 | HAWK_FMT_UINTMAX_NOTRUNC | HAWK_FMT_UINTMAX_NONULL,
			-1,
			HAWK_T('\0'),
			HAWK_NULL
		);

		if (hawk_rtx_setmapvalfld(rtx, rmap, ridx, ridx_len, va[i]) == HAWK_NULL)
		{
			/* decrement the reference count of the values not added to the map */
			do
			{
				hawk_rtx_refdownval (rtx, va[i]);
				i++;
			}
			while (i < msz);
			hawk_rtx_freeval (rtx, rmap, 0); /* this derefs the elements added. */
			hawk_rtx_freemem (rtx, va);
			return -1;
		}

		hawk_rtx_refdownval (rtx, va[i]); /* deref it as it has been added to the map */
	}

	rv = msz;
	hawk_rtx_freemem (rtx, va);

done:
	r = hawk_rtx_makeintval(rtx, rv);
	if (!r) return -1;

	if (rmap)
	{
		/* rmap can be NULL when a jump has been made for an empty source 
		 * at the beginning of this fucntion */
		hawk_rtx_refupval (rtx, rmap);
		x = hawk_rtx_setrefval(rtx, (hawk_val_ref_t*)hawk_rtx_getarg(rtx, (nargs >= 2)), rmap);
		hawk_rtx_refdownval (rtx, rmap);
		if (x <= -1) 
		{
			hawk_rtx_freeval (rtx, r, 0);
			return -1;
		}
	}

	hawk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_asort (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	return __fnc_asort(rtx, fi, 0);
}

static int fnc_asorti (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	return __fnc_asort(rtx, fi, 1);
}
