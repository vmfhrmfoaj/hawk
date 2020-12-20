/*
 * $Id$
 *
    Copyright (c) 2006-2020 Chung, Hyung-Hwan. All rights reserved.

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

#include "mod-ffi.h"
#include <hawk-utl.h>
#include "../lib/hawk-prv.h"

#include <errno.h>
#include <string.h>

#if defined(HAVE_FFI_LIB) && defined(HAVE_FFI_H)
#	define USE_LIBFFI
#elif defined(HAVE_DYNCALL_LIB) && defined(HAVE_DYNCALL_H)
#	define USE_DYNCALL
#endif

#if defined(USE_LIBFFI)
#	include <ffi.h>
#	if (HAWK_SIZEOF_LONG_LONG > 0) && !defined(ffi_type_ulonglong)
#		if HAWK_SIZEOF_LONG_LONG == HAWK_SIZEOF_INT32_T
#			define ffi_type_ulonglong ffi_type_uint32
#			define ffi_type_slonglong ffi_type_sint32
#		elif HAWK_SIZEOF_LONG_LONG == HAWK_SIZEOF_INT64_T
#			define ffi_type_ulonglong ffi_type_uint64
#			define ffi_type_slonglong ffi_type_sint64
#		endif
#	endif
#elif defined(USE_DYNCALL)
#	include <dyncall.h>

#define __dcArgInt8 dcArgChar
#define __dcCallInt8 dcCallChar

#define __dcArgInt16 dcArgShort
#define __dcCallInt16 dcCallShort

#if (HAWK_SIZEOF_INT32_T == HAWK_SIZEOF_INT)
#define __dcArgInt32 dcArgInt
#define __dcCallInt32 dcCallInt
#elif (HAWK_SIZEOF_INT32_T == HAWK_SIZEOF_LONG)
#define __dcArgInt32 dcArgLong
#define __dcCallInt32 dcCallLong
#endif

#if (HAWK_SIZEOF_INT64_T == HAWK_SIZEOF_LONG)
#define __dcArgInt64 dcArgLong
#define __dcCallInt64 dcCallLong
#elif (HAWK_SIZEOF_INT64_T == HAWK_SIZEOF_LONG_LONG)
#define __dcArgInt64 dcArgLongLong
#define __dcCallInt64 dcCallLongLong
#endif

#endif /* USE_DYNCALL */

#define FMTC_NULL '\0' /* internal use only */

#define FMTC_CHAR 'c'
#define FMTC_SHORT 'h'
#define FMTC_INT 'i'
#define FMTC_LONG 'l'
#define FMTC_LONGLONG 'L'
#define FMTC_BCS 's'
#define FMTC_UCS 'S'
#define FMTC_BLOB 'b'
#define FMTC_POINTER 'p'

#define FMTC_INT8 '1'
#define FMTC_INT16 '2'
#define FMTC_INT32 '4'
#define FMTC_INT64 '8'

typedef struct link_t link_t;
struct link_t
{
	link_t* next;
};

#if defined(USE_LIBFFI)
typedef union ffi_sv_t ffi_sv_t;
union ffi_sv_t
{
	void* p;
	unsigned char uc;
	char c;
	unsigned short int uh;
	short h;
	unsigned int ui;
	int i;
	unsigned long int ul;
	long int l;
#if (HAWK_SIZEOF_LONG_LONG > 0)
	unsigned long long int ull;
	long long int ll;
#endif

	hawk_uint8_t ui8;
	hawk_int8_t i8;
	hawk_uint16_t ui16;
	hawk_int16_t i16;
	hawk_uint32_t ui32;
	hawk_int32_t i32;
#if (HAWK_SIZEOF_INT64_T > 0)
	hawk_uint64_t ui64;
	hawk_int64_t i64;
#endif
};
#endif

typedef struct ffi_t ffi_t;
struct ffi_t
{
	void* handle;

#if defined(USE_DYNCALL)
	DCCallVM* dc;
#elif defined(USE_LIBFFI)
	hawk_oow_t arg_count;
	hawk_oow_t arg_max;
	ffi_type** arg_types;
	void** arg_values;
	ffi_sv_t* arg_svs;

	ffi_sv_t ret_sv;
	ffi_cif cif;
	ffi_type* fmtc_to_type[2][128];
#endif

	link_t* ca; /* call arguments duplicated */
};


#define __IDMAP_NODE_T_DATA  ffi_t ffi;
#define __IDMAP_LIST_T_DATA  hawk_ooch_t errmsg[256];
#define __IDMAP_LIST_T ffi_list_t
#define __IDMAP_NODE_T ffi_node_t
#define __INIT_IDMAP_LIST __init_ffi_list
#define __FINI_IDMAP_LIST __fini_ffi_list
#define __MAKE_IDMAP_NODE __new_ffi_node
#define __FREE_IDMAP_NODE __free_ffi_node
#include "../lib/idmap-imp.h"

struct rtx_data_t
{
	ffi_list_t ffi_list;

};
typedef struct rtx_data_t rtx_data_t;

/* ------------------------------------------------------------------------ */
#define ERRNUM_TO_RC(errnum) (-((hawk_int_t)errnum))

static hawk_int_t copy_error_to_ffi_list (hawk_rtx_t* rtx, ffi_list_t* ffi_list)
{
	hawk_errnum_t errnum = hawk_rtx_geterrnum(rtx);
	hawk_copy_oocstr (ffi_list->errmsg, HAWK_COUNTOF(ffi_list->errmsg), hawk_rtx_geterrmsg(rtx));
	return ERRNUM_TO_RC(errnum);
}

static hawk_int_t set_error_on_ffi_list_with_errno (hawk_rtx_t* rtx, ffi_list_t* ffi_list, const hawk_ooch_t* title)
{
	int err = errno;

	if (title)
		 hawk_rtx_fmttooocstr (rtx, ffi_list->errmsg, HAWK_COUNTOF(ffi_list->errmsg), HAWK_T("%js - %hs"), title, strerror(err));
	else
		 hawk_rtx_fmttooocstr (rtx, ffi_list->errmsg, HAWK_COUNTOF(ffi_list->errmsg), HAWK_T("%hs"), strerror(err));
	return ERRNUM_TO_RC(hawk_syserr_to_errnum(err));
}


static hawk_int_t set_error_on_ffi_list (hawk_rtx_t* rtx, ffi_list_t* ffi_list, hawk_errnum_t errnum, const hawk_ooch_t* errfmt, ...)
{
	va_list ap;
	if (errfmt)
	{
		va_start (ap, errfmt);
		hawk_rtx_vfmttooocstr (rtx, ffi_list->errmsg, HAWK_COUNTOF(ffi_list->errmsg), errfmt, ap);
		va_end (ap);
	}
	else
	{
		hawk_rtx_fmttooocstr (rtx, ffi_list->errmsg, HAWK_COUNTOF(ffi_list->errmsg), HAWK_T("%js"), hawk_geterrstr(hawk_rtx_gethawk(rtx))(errnum));
	}
	return ERRNUM_TO_RC(errnum);
}

static void set_errmsg_on_ffi_list (hawk_rtx_t* rtx, ffi_list_t* ffi_list, const hawk_ooch_t* errfmt, ...)
{
	if (errfmt)
	{
		va_list ap;
		va_start (ap, errfmt);
		hawk_rtx_vfmttooocstr (rtx, ffi_list->errmsg, HAWK_COUNTOF(ffi_list->errmsg), errfmt, ap);
		va_end (ap);
	}
	else
	{
		hawk_copy_oocstr (ffi_list->errmsg, HAWK_COUNTOF(ffi_list->errmsg), hawk_rtx_geterrmsg(rtx));
	}
}

/* ------------------------------------------------------------------------ */

static HAWK_INLINE rtx_data_t* rtx_to_data (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_rbt_pair_t* pair;
	pair = hawk_rbt_search((hawk_rbt_t*)fi->mod->ctx, &rtx, HAWK_SIZEOF(rtx));
	HAWK_ASSERT (pair != HAWK_NULL);
	return (rtx_data_t*)HAWK_RBT_VPTR(pair);
}

static HAWK_INLINE ffi_list_t* rtx_to_ffi_list (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	return &rtx_to_data(rtx, fi)->ffi_list;
}

/* ------------------------------------------------------------------------ */

static HAWK_INLINE void link_ca (ffi_t* ffi, void* ptr)
{
	link_t* l = (link_t*)((hawk_oob_t*)ptr - HAWK_SIZEOF_VOID_P);
	l->next = ffi->ca;
	ffi->ca = l;
}

static void free_linked_cas (hawk_rtx_t* rtx, ffi_t* ffi)
{
	while (ffi->ca)
	{
		link_t* ptr;
		ptr = ffi->ca;
		ffi->ca = ptr->next;
		hawk_rtx_freemem (rtx, ptr);
	}
}

/* ------------------------------------------------------------------------ */

static ffi_node_t* new_ffi_node (hawk_rtx_t* rtx, ffi_list_t* ffi_list, const hawk_ooch_t* name)
{
	hawk_t* hawk = hawk_rtx_gethawk(rtx);
	ffi_node_t* ffi_node;
	hawk_mod_spec_t spec;
	void* handle;
#if defined(USE_DYNCALL)
	DCCallVM* dc;
#endif

	ffi_node = __new_ffi_node(rtx, ffi_list);
	if (!ffi_node) return HAWK_NULL;

	HAWK_MEMSET (&spec, 0, HAWK_SIZEOF(spec));
	spec.name = name;

	handle = hawk->prm.modopen(hawk, &spec);
	if (!handle) 
	{
		const hawk_ooch_t* olderrmsg;
		olderrmsg = hawk_backuperrmsg(hawk);
		hawk_rtx_seterrfmt (rtx, HAWK_NULL, HAWK_ENOMEM, HAWK_T("unable to open the '%js' ffi module - %js"), name, olderrmsg);

		__free_ffi_node (rtx, ffi_list, ffi_node);
		return HAWK_NULL;
	}

#if defined(USE_DYNCALL)
	dc = dcNewCallVM(4096); /* TODO: right size?  */
	if (!dc) 
	{
		hawk_rtx_seterrwithsyserr (hawk, 0, errno);
		hawk->prm.modclose (hawk, handle);
		__free_ffi_node (rtx, ffi_list, ffi_node);
		return HAWK_NULL;
	}
#endif

	ffi_node->ffi.handle = handle;

#if defined(USE_DYNCALL)
	ffi_node->ffi.dc = dc;
#elif defined(USE_LIBFFI)
	ffi_node->ffi.fmtc_to_type[0][FMTC_NULL] = &ffi_type_void;
	ffi_node->ffi.fmtc_to_type[1][FMTC_NULL] = &ffi_type_void;

	ffi_node->ffi.fmtc_to_type[0][FMTC_CHAR] = &ffi_type_schar;
	ffi_node->ffi.fmtc_to_type[1][FMTC_CHAR] = &ffi_type_uchar;
	ffi_node->ffi.fmtc_to_type[0][FMTC_SHORT] = &ffi_type_sshort;
	ffi_node->ffi.fmtc_to_type[1][FMTC_SHORT] = &ffi_type_ushort;
	ffi_node->ffi.fmtc_to_type[0][FMTC_INT] = &ffi_type_sint;
	ffi_node->ffi.fmtc_to_type[1][FMTC_INT] = &ffi_type_uint;
	ffi_node->ffi.fmtc_to_type[0][FMTC_LONG] = &ffi_type_slong;
	ffi_node->ffi.fmtc_to_type[1][FMTC_LONG] = &ffi_type_ulong;
	#if (HAWK_SIZEOF_LONG_LONG > 0)
	ffi_node->ffi.fmtc_to_type[0][FMTC_LONGLONG] = &ffi_type_slonglong;
	ffi_node->ffi.fmtc_to_type[1][FMTC_LONGLONG] = &ffi_type_ulonglong;
	#endif
	ffi_node->ffi.fmtc_to_type[0][FMTC_INT8] = &ffi_type_sint8;
	ffi_node->ffi.fmtc_to_type[1][FMTC_INT8] = &ffi_type_uint8;
	ffi_node->ffi.fmtc_to_type[0][FMTC_INT16] = &ffi_type_sint16;
	ffi_node->ffi.fmtc_to_type[1][FMTC_INT16] = &ffi_type_uint16;
	ffi_node->ffi.fmtc_to_type[0][FMTC_INT32] = &ffi_type_sint32;
	ffi_node->ffi.fmtc_to_type[1][FMTC_INT32] = &ffi_type_uint32;
	ffi_node->ffi.fmtc_to_type[0][FMTC_INT64] = &ffi_type_sint64;
	ffi_node->ffi.fmtc_to_type[1][FMTC_INT64] = &ffi_type_uint64;

	ffi_node->ffi.fmtc_to_type[0][FMTC_POINTER] = &ffi_type_pointer;
	ffi_node->ffi.fmtc_to_type[1][FMTC_POINTER] = &ffi_type_pointer;
	ffi_node->ffi.fmtc_to_type[0][FMTC_BCS] = &ffi_type_pointer;
	ffi_node->ffi.fmtc_to_type[1][FMTC_BCS] = &ffi_type_pointer;
	ffi_node->ffi.fmtc_to_type[0][FMTC_UCS] = &ffi_type_pointer;
	ffi_node->ffi.fmtc_to_type[1][FMTC_UCS] = &ffi_type_pointer;
#endif

	return ffi_node;
}

static void free_ffi_node (hawk_rtx_t* rtx, ffi_list_t* ffi_list, ffi_node_t* ffi_node)
{
	hawk_t* hawk = hawk_rtx_gethawk(rtx);

	free_linked_cas (rtx, &ffi_node->ffi);

#if defined(USE_DYNCALL)
	dcFree (ffi_node->ffi.dc);
	ffi_node->ffi.dc = HAWK_NULL;
#elif defined(USE_LIBFFI)
	if (ffi_node->ffi.arg_types)
	{
		hawk_rtx_freemem (rtx, ffi_node->ffi.arg_types);
		ffi_node->ffi.arg_types = HAWK_NULL;
	}
	if (ffi_node->ffi.arg_values)
	{
		hawk_rtx_freemem (rtx, ffi_node->ffi.arg_values);
		ffi_node->ffi.arg_values = HAWK_NULL;
	}
	if (ffi_node->ffi.arg_svs)
	{
		hawk_rtx_freemem (rtx, ffi_node->ffi.arg_svs);
		ffi_node->ffi.arg_svs = HAWK_NULL;
	}
	ffi_node->ffi.arg_max = 0;
	ffi_node->ffi.arg_count = 0;
#endif

	hawk->prm.modclose (hawk, ffi_node->ffi.handle);
	ffi_node->ffi.handle = HAWK_NULL;

	__free_ffi_node (rtx, ffi_list, ffi_node);
}

static HAWK_INLINE ffi_node_t* get_ffi_list_node (ffi_list_t* ffi_list, hawk_int_t id)
{
	if (id < 0 || id >= ffi_list->map.high || !ffi_list->map.tab[id]) return HAWK_NULL;
	return ffi_list->map.tab[id];
}

static ffi_node_t* get_ffi_list_node_with_arg (hawk_rtx_t* rtx, ffi_list_t* ffi_list, hawk_val_t* arg, hawk_int_t* rx)
{
	hawk_int_t id;
	ffi_node_t* ffi_node;


	if (hawk_rtx_valtoint(rtx, arg, &id) <= -1)
	{
		 *rx = ERRNUM_TO_RC(hawk_rtx_geterrnum(rtx));
		 set_errmsg_on_ffi_list (rtx, ffi_list, HAWK_T("illegal handle value"));
		 return HAWK_NULL;
	}
	else if (!(ffi_node = get_ffi_list_node(ffi_list, id)))
	{
		 *rx = ERRNUM_TO_RC(HAWK_EINVAL);
		 set_errmsg_on_ffi_list (rtx, ffi_list, HAWK_T("invalid handle - %zd"), (hawk_oow_t)id);
		 return HAWK_NULL;
	}

	return ffi_node;
}

/* ------------------------------------------------------------------------ */

static int fnc_open (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	ffi_list_t* ffi_list;
	ffi_node_t* ffi_node = HAWK_NULL;
	hawk_int_t ret = -1;
	hawk_val_t* retv;
	hawk_val_t* a0;
	hawk_oocs_t name;

	ffi_list = rtx_to_ffi_list(rtx, fi);

	a0 = hawk_rtx_getarg(rtx, 0);
	name.ptr = hawk_rtx_getvaloocstr(rtx, a0, &name.len);
	if (HAWK_UNLIKELY(!name.ptr))
	{
		ret = copy_error_to_ffi_list (rtx, ffi_list);
		goto done;
	}

	if (name.len != hawk_count_oocstr(name.ptr))
	{
		ret = set_error_on_ffi_list(rtx, ffi_list, HAWK_EINVAL, HAWK_T("invalid ffi module name '%.*js'"), name.len, name.ptr);
		goto done;
	}

	ffi_node = new_ffi_node(rtx, ffi_list, name.ptr);
	if (ffi_node) ret = ffi_node->id;
	else ret = copy_error_to_ffi_list (rtx, ffi_list);

done:
	if (name.ptr) hawk_rtx_freevaloocstr (rtx, a0, name.ptr);

	retv = hawk_rtx_makeintval(rtx, ret);
	if (HAWK_UNLIKELY(!retv))
	{
		if (ffi_node) free_ffi_node (rtx, ffi_list, ffi_node);
		return -1;
	}

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_close (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_t* hawk = hawk_rtx_gethawk(rtx);
	ffi_list_t* ffi_list;
	ffi_node_t* ffi_node;
	hawk_int_t ret = -1;

	ffi_list = rtx_to_ffi_list(rtx, fi);
	ffi_node = get_ffi_list_node_with_arg(rtx, ffi_list, hawk_rtx_getarg(rtx, 0), &ret);
	if (ffi_node)
	{
		free_linked_cas (rtx, &ffi_node->ffi);

	#if defined(USE_DYNCALL)
		dcFree (ffi_node->ffi.dc);
		ffi_node->ffi.dc = HAWK_NULL;
	#elif defined(USE_LIBFFI)
		if (ffi_node->ffi.arg_types)
		{
			hawk_rtx_freemem (rtx, ffi_node->ffi.arg_types);
			ffi_node->ffi.arg_types = HAWK_NULL;
		}
		if (ffi_node->ffi.arg_values)
		{
			hawk_rtx_freemem (rtx, ffi_node->ffi.arg_values);
			ffi_node->ffi.arg_values = HAWK_NULL;
		}
		if (ffi_node->ffi.arg_svs)
		{
			hawk_rtx_freemem (rtx, ffi_node->ffi.arg_svs);
			ffi_node->ffi.arg_svs = HAWK_NULL;
		}
		ffi_node->ffi.arg_max = 0;
		ffi_node->ffi.arg_count = 0;
	#endif

		hawk->prm.modclose (hawk, ffi_node->ffi.handle);
		ffi_node->ffi.handle = HAWK_NULL;
	}

	hawk_rtx_setretval (rtx, hawk_rtx_makeintval(rtx, ret));
	return 0;
}

#if 0
static HAWK_INLINE int add_ffi_arg (hawk_t* hawk, ffi_t* ffi, hawk_ooch_t fmtc, int _unsigned, hawk_oop_t arg)
{
#if defined(USE_LIBFFI)
	if (ffi->arg_count >= ffi->arg_max)
	{
		ffi_type** ttmp;
		void** vtmp;
		ffi_sv_t* stmp;

		hawk_oow_t newmax;

		newmax = ffi->arg_max + 16; /* TODO: adjust this? */
		ttmp = hawk_reallocmem(hawk, ffi->arg_types, HAWK_SIZEOF(*ttmp) * newmax);
		if (HAWK_UNLIKELY(!ttmp)) goto oops;
		vtmp = hawk_reallocmem(hawk, ffi->arg_values, HAWK_SIZEOF(*vtmp) * newmax);
		if (HAWK_UNLIKELY(!vtmp)) goto oops;
		stmp = hawk_reallocmem(hawk, ffi->arg_svs, HAWK_SIZEOF(*stmp) * newmax);
		if (HAWK_UNLIKELY(!stmp)) goto oops;

		ffi->arg_types = ttmp;
		ffi->arg_values = vtmp;
		ffi->arg_svs = stmp;
		ffi->arg_max = newmax;
	}
#endif

	switch (fmtc)
	{
		case FMTC_INT8:
			if (_unsigned)
			{
				hawk_oow_t v;
				if (HAWK_OOP_IS_CHAR(arg)) v = HAWK_OOP_TO_CHAR(arg);
				else if (hawk_inttooow_noseterr(hawk, arg, &v) <= 0) goto inval_arg_value;;
			#if defined(USE_DYNCALL)
				__dcArgInt8 (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].ui8;
				ffi->arg_svs[ffi->arg_count].ui8 = v;
			#endif
			}
			else
			{
				hawk_ooi_t v;
				if (HAWK_OOP_IS_CHAR(arg)) v = HAWK_OOP_TO_CHAR(arg);
				else if (hawk_inttoooi_noseterr(hawk, arg, &v) == 0) goto inval_arg_value;;
			#if defined(USE_DYNCALL)
				__dcArgInt8 (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].i8;
				ffi->arg_svs[ffi->arg_count].i8 = v;
			#endif
			}
			break;

		case FMTC_INT16:
			if (_unsigned)
			{
				hawk_oow_t v;
				if (hawk_inttooow_noseterr(hawk, arg, &v) <= 0) goto inval_arg_value;;
			#if defined(USE_DYNCALL)
				__dcArgInt16 (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].ui16;
				ffi->arg_svs[ffi->arg_count].ui16 = v;
			#endif
			}
			else
			{
				hawk_ooi_t v;
				if (hawk_inttoooi_noseterr(hawk, arg, &v) == 0) goto inval_arg_value;;
			#if defined(USE_DYNCALL)
				__dcArgInt16 (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].i16;
				ffi->arg_svs[ffi->arg_count].i16 = v;
			#endif
			}
			break;


		case FMTC_INT32:
			if (_unsigned)
			{
				hawk_oow_t v;
				if (hawk_inttooow_noseterr(hawk, arg, &v) <= 0) goto inval_arg_value;;
			#if defined(USE_DYNCALL)
				__dcArgInt32 (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].ui32;
				ffi->arg_svs[ffi->arg_count].ui32 = v;
			#endif
			}
			else
			{
				hawk_ooi_t v;
				if (hawk_inttoooi_noseterr(hawk, arg, &v) == 0) goto inval_arg_value;;
			#if defined(USE_DYNCALL)
				__dcArgInt32 (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].i32;
				ffi->arg_svs[ffi->arg_count].i32 = v;
			#endif
			}
			break;


		case FMTC_INT64:
		#if (HAWK_SIZEOF_INT64_T > 0)
			if (_unsigned)
			{
				hawk_oow_t v;
				if (hawk_inttooow_noseterr(hawk, arg, &v) <= 0) goto inval_arg_value;
			#if defined(USE_DYNCALL)
				__dcArgInt64 (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].ui64;
				ffi->arg_svs[ffi->arg_count].ui64 = v;
			#endif
			}
			else
			{
				hawk_ooi_t v;
				if (hawk_inttoooi_noseterr(hawk, arg, &v) == 0) goto inval_arg_value;
			#if defined(USE_DYNCALL)
				__dcArgInt64 (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].i64;
				ffi->arg_svs[ffi->arg_count].i64 = v;
			#endif
			}
			break;
		#else
			goto inval_sig_ch;
		#endif

		case FMTC_CHAR: /* this is a byte */
			if (_unsigned)
			{
				hawk_oow_t v;
				if (HAWK_OOP_IS_CHAR(arg)) v = HAWK_OOP_TO_CHAR(arg);
				else if (hawk_inttooow_noseterr(hawk, arg, &v) <= 0) goto inval_arg_value;;
			#if defined(USE_DYNCALL)
				dcArgChar (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].uc;
				ffi->arg_svs[ffi->arg_count].uc = v;
			#endif
			}
			else
			{
				hawk_ooi_t v;
				if (HAWK_OOP_IS_CHAR(arg)) v = HAWK_OOP_TO_CHAR(arg);
				else if (hawk_inttoooi_noseterr(hawk, arg, &v) == 0) goto inval_arg_value;;
			#if defined(USE_DYNCALL)
				dcArgChar (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].c;
				ffi->arg_svs[ffi->arg_count].c = v;
			#endif
			}
			break;

		case FMTC_SHORT:
			if (_unsigned)
			{
				hawk_oow_t v;
				if (hawk_inttooow_noseterr(hawk, arg, &v) <= 0) goto inval_arg_value;
			#if defined(USE_DYNCALL)
				dcArgShort (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].uh;
				ffi->arg_svs[ffi->arg_count].uh = v;
			#endif
			}
			else
			{
				hawk_ooi_t v;
				if (hawk_inttoooi_noseterr(hawk, arg, &v) == 0) goto inval_arg_value;
			#if defined(USE_DYNCALL)
				dcArgShort (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].h;
				ffi->arg_svs[ffi->arg_count].h = v;
			#endif
			}
			break;

		case FMTC_INT:
			if (_unsigned)
			{
				hawk_oow_t v;
				if (hawk_inttooow_noseterr(hawk, arg, &v) <= 0) goto inval_arg_value;
			#if defined(USE_DYNCALL)
				dcArgInt (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].ui;
				ffi->arg_svs[ffi->arg_count].ui = v;
			#endif
			}
			else
			{
				hawk_ooi_t v;
				if (hawk_inttoooi_noseterr(hawk, arg, &v) == 0) goto inval_arg_value;
			#if defined(USE_DYNCALL)
				dcArgInt (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].i;
				ffi->arg_svs[ffi->arg_count].i = v;
			#endif
			}
			break;

		case FMTC_LONG:
		arg_as_long:
			if (_unsigned)
			{
				hawk_oow_t v;
				if (hawk_inttooow_noseterr(hawk, arg, &v) <= 0) goto inval_arg_value;
			#if defined(USE_DYNCALL)
				dcArgLong (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].ul;
				ffi->arg_svs[ffi->arg_count].ul = v;
			#endif
			}
			else 
			{
				hawk_ooi_t v;
				if (hawk_inttoooi_noseterr(hawk, arg, &v) == 0) goto inval_arg_value;;
			#if defined(USE_DYNCALL)
				dcArgLong (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].l;
				ffi->arg_svs[ffi->arg_count].l = v;
			#endif
			}
			break;

		case FMTC_LONGLONG:
		#if (HAWK_SIZEOF_LONG_LONG <= HAWK_SIZEOF_LONG)
			goto arg_as_long;
		#else
			if (_unsigned)
			{
				hawk_uintmax_t v;
				if (hawk_inttouintmax_noseterr(hawk, arg, &v) <= 0) goto inval_arg_value;
			#if defined(USE_DYNCALL)
				dcArgLongLong (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].ull;
				ffi->arg_svs[ffi->arg_count].ull = v;
			#endif
			}
			else
			{
				hawk_intmax_t v;
				if (hawk_inttointmax_noseterr(hawk, arg, &v) == 0) goto inval_arg_value;
			#if defined(USE_DYNCALL)
				dcArgLongLong (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].ll;
				ffi->arg_svs[ffi->arg_count].ll = v;
			#endif
			}
			break;
		#endif

		case FMTC_BCS:
		{
			hawk_bch_t* ptr;

			if (!HAWK_OBJ_IS_CHAR_POINTER(arg)) goto inval_arg_value;

		#if defined(HAWK_OOCH_IS_UCH)
			ptr = hawk_dupootobcharswithheadroom(hawk, HAWK_SIZEOF_VOID_P, HAWK_OBJ_GET_CHAR_SLOT(arg), HAWK_OBJ_GET_SIZE(arg), HAWK_NULL);
			if (!ptr) goto oops; /* out of system memory or conversion error - soft failure */
			link_ca (ffi, ptr);
		#else
			ptr = HAWK_OBJ_GET_CHAR_SLOT(arg);
			/*ptr = hawk_dupoochars(hawk, HAWK_OBJ_GET_CHAR_SLOT(arg), HAWK_OBJ_GET_SIZE(arg));
			if (!ptr) goto oops;*/ /* out of system memory or conversion error - soft failure */
		#endif

		#if defined(USE_DYNCALL)
			dcArgPointer (ffi->dc, ptr);
		#elif defined(USE_LIBFFI)
			ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].p;
			ffi->arg_svs[ffi->arg_count].p = ptr;
		#endif
			break;
		}

		case FMTC_UCS:
		{
			hawk_uch_t* ptr;

			if (!HAWK_OBJ_IS_CHAR_POINTER(arg)) goto inval_arg_value;

		#if defined(HAWK_OOCH_IS_UCH)
			ptr = HAWK_OBJ_GET_CHAR_SLOT(arg);
			/*ptr = hawk_dupoochars(hawk, HAWK_OBJ_GET_CHAR_SLOT(arg), HAWK_OBJ_GET_SIZE(arg));
			if (!ptr) goto oops; */ /* out of system memory or conversion error - soft failure */
		#else
			ptr = hawk_dupootoucharswithheadroom(hawk, HAWK_SIZEOF_VOID_P, HAWK_OBJ_GET_CHAR_SLOT(arg), HAWK_OBJ_GET_SIZE(arg), HAWK_NULL);
			if (!ptr) goto oops; /* out of system memory or conversion error - soft failure */
			link_ca (ffi, ptr);
		#endif

		#if defined(USE_DYNCALL)
			dcArgPointer (ffi->dc, ptr);
		#elif defined(USE_LIBFFI)
			ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].p;
			ffi->arg_svs[ffi->arg_count].p = ptr;
		#endif
			break;
		}

		case FMTC_BLOB:
		{
			void* ptr;

			if (HAWK_OBJ_IS_BYTE_POINTER(arg)) goto inval_arg_value;
			ptr = HAWK_OBJ_GET_BYTE_SLOT(arg);

		#if defined(USE_DYNCALL)
			dcArgPointer (ffi->dc, ptr);
		#elif defined(USE_LIBFFI)
			ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].p;
			ffi->arg_svs[ffi->arg_count].p = ptr;
		#endif
			break;
		}

		case FMTC_POINTER:
		{
			void* ptr;

			/* TODO: map nil to NULL? or should #\p0 be enough? */
			if (!HAWK_OOP_IS_SMPTR(arg)) goto inval_arg_value;
			ptr = HAWK_OOP_TO_SMPTR(arg);

		#if defined(USE_DYNCALL)
			dcArgPointer (ffi->dc, ptr);
		#elif defined(USE_LIBFFI)
			ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].p;
			ffi->arg_svs[ffi->arg_count].p = ptr;
		#endif
			break;
		}

		default:
		inval_sig_ch:
			/* invalid argument signature specifier */
			hawk_seterrbfmt (hawk, HAWK_EINVAL, "invalid argument type signature - %jc", fmtc);
			goto oops;
	}

#if defined(USE_LIBFFI)
	ffi->arg_types[ffi->arg_count] = ffi->fmtc_to_type[_unsigned][fmtc];
	ffi->arg_count++;
#endif
	return 0;

inval_arg_value:
	hawk_seterrbfmt (hawk, HAWK_EINVAL, "invalid argument value - %O", arg);

oops:
	return -1;
}


static hawk_pfrc_t fnc_call (hawk_t* hawk, hawk_mod_t* mod, hawk_ooi_t nargs)
{
#if defined(USE_DYNCALL) || defined(USE_LIBFFI)
	ffi_t* ffi;
	hawk_oop_t fun, sig, args;
	hawk_oow_t i, j, nfixedargs, _unsigned;
	void* f;
	hawk_oop_oop_t arr;
	int vbar = 0;
	hawk_ooch_t fmtc;
	#if defined(USE_LIBFFI)
	ffi_status fs;
	#endif

	ffi = (ffi_t*)hawk_getobjtrailer(hawk, HAWK_STACK_GETRCV(hawk, nargs), HAWK_NULL);

	fun = HAWK_STACK_GETARG(hawk, nargs, 0);
	sig = HAWK_STACK_GETARG(hawk, nargs, 1);
	args = HAWK_STACK_GETARG(hawk, nargs, 2);

	if (hawk_ptrtooow(hawk, fun, (hawk_oow_t*)&f) <= -1) goto softfail;

	/* the signature must not be empty. at least the return type must be
	 * specified */
	if (!HAWK_OBJ_IS_CHAR_POINTER(sig) || HAWK_OBJ_GET_SIZE(sig) <= 0) goto inval;

#if 0
	/* TODO: check if arr is a kind of array??? or check if it's indexed */
	if (HAWK_OBJ_GET_SIZE(sig) > 1 && HAWK_CLASSOF(hawk,args) != hawk->_array) goto inval;
#endif

	arr = (hawk_oop_oop_t)args;
	/*HAWK_DEBUG2 (hawk, "<ffi.call> %p in %p\n", f, ffi->handle);*/

#if defined(USE_DYNCALL)
	dcMode (ffi->dc, DC_CALL_C_DEFAULT);
	dcReset (ffi->dc);

	for (i = 0; i < HAWK_OBJ_GET_SIZE(sig); i++)
	{
		fmtc = HAWK_OBJ_GET_CHAR_VAL(sig, i);
		if (fmtc == '>') break; /* end of arguments. start of return type */
		if (fmtc == '|')
		{
			dcMode (ffi->dc, DC_CALL_C_ELLIPSIS); /* variadic. for arguments before ... */
			/* the error code should be DC_ERROR_UNSUPPORTED_MODE */
			if (dcGetError(ffi->dc) != DC_ERROR_NONE) goto noimpl;
			dcReset (ffi->dc);
			break;
		}
	}
#else
	ffi->arg_count = 0;
#endif

	/* check argument signature */
	for (i = 0, j = 0, nfixedargs = 0, _unsigned = 0; i < HAWK_OBJ_GET_SIZE(sig); i++)
	{
		fmtc = HAWK_OBJ_GET_CHAR_VAL(sig, i);
		if (fmtc == ' ')
		{
			continue;
		}
		if (fmtc == '>') 
		{
			i++;
			if (!vbar) nfixedargs = j;
			break;
		}
		else if (fmtc == '|')
		{
			if (!vbar)
			{
			#if defined(USE_DYNCALL)
				dcMode (ffi->dc, DC_CALL_C_ELLIPSIS_VARARGS); /* start of arguments that fall to the ... part */
				/* the error code should be DC_ERROR_UNSUPPORTED_MODE */
				if (dcGetError(ffi->dc) != DC_ERROR_NONE) goto noimpl;
			#endif
				nfixedargs = j;
				vbar = 1;
			}
			continue;
		}
		else if (fmtc == 'u')
		{
			_unsigned = 1;
			continue;
		}

		/* more items in signature than the actual argument */  
		if (j >= HAWK_OBJ_GET_SIZE(arr)) goto inval;

		if (add_ffi_arg(hawk, ffi, fmtc, _unsigned, HAWK_OBJ_GET_OOP_VAL(arr, j)) <= -1) goto softfail;
		_unsigned = 0;
		j++;
	}

	while (i < HAWK_OBJ_GET_SIZE(sig) && HAWK_OBJ_GET_CHAR_VAL(sig, i) == ' ') i++; /* skip all spaces after > */

	fmtc = (i >= HAWK_OBJ_GET_SIZE(sig)? FMTC_NULL: HAWK_OBJ_GET_CHAR_VAL(sig, i));
#if defined(USE_LIBFFI)
	fs = (nfixedargs == j)? ffi_prep_cif(&ffi->cif, FFI_DEFAULT_ABI, j, ffi->fmtc_to_type[0][fmtc], ffi->arg_types):
	                        ffi_prep_cif_var(&ffi->cif, FFI_DEFAULT_ABI, nfixedargs, j, ffi->fmtc_to_type[0][fmtc], ffi->arg_types);
	if (fs != FFI_OK)
	{
		hawk_seterrnum (hawk, HAWK_ESYSERR);
		goto softfail;
	}

	ffi_call (&ffi->cif, FFI_FN(f), &ffi->ret_sv, ffi->arg_values);
#endif

	if (fmtc == 'u') 
	{
		_unsigned = 1;
		fmtc++;
	}
	else
	{
		_unsigned = 0;
	}

	/* check the return value type in signature */
	switch (fmtc)
	{
/* TODO: support more types... */
/* TODO: proper return value conversion */
		case FMTC_INT8:
		{
			hawk_oop_t r;
			if (_unsigned)
			{
			#if defined(USE_DYNCALL)
				r = hawk_oowtoint(hawk, __dcCallInt8(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_oowtoint(hawk, ffi->ret_sv.ui8);
			#endif
			}
			else
			{
			#if defined(USE_DYNCALL)
				r = hawk_ooitoint(hawk, __dcCallInt8(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_ooitoint(hawk, ffi->ret_sv.i8);
			#endif
			}
			if (!r) goto hardfail;
		
			HAWK_STACK_SETRET (hawk, nargs, r);
			break;
		}

		case FMTC_INT16:
		{
			hawk_oop_t r;
			if (_unsigned)
			{
			#if defined(USE_DYNCALL)
				r = hawk_oowtoint(hawk, __dcCallInt16(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_oowtoint(hawk, ffi->ret_sv.ui16);
			#endif
			}
			else
			{
			#if defined(USE_DYNCALL)
				r = hawk_ooitoint(hawk, __dcCallInt16(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_ooitoint(hawk, ffi->ret_sv.i16);
			#endif
			}
			if (!r) goto hardfail;
		
			HAWK_STACK_SETRET (hawk, nargs, r);
			break;
		}

		case FMTC_INT32:
		{
			hawk_oop_t r;
			if (_unsigned)
			{
			#if defined(USE_DYNCALL)
				r = hawk_oowtoint(hawk, __dcCallInt32(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_oowtoint(hawk, ffi->ret_sv.ui32);
			#endif
			}
			else
			{
			#if defined(USE_DYNCALL)
				r = hawk_ooitoint(hawk, __dcCallInt32(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_ooitoint(hawk, ffi->ret_sv.i32);
			#endif
			}
			if (!r) goto hardfail;
		
			HAWK_STACK_SETRET (hawk, nargs, r);
			break;
		}

		case FMTC_INT64:
		{
		#if (HAWK_SIZEOF_INT64_T > 0)
			hawk_oop_t r;
			if (_unsigned)
			{
			#if defined(USE_DYNCALL)
				r = hawk_oowtoint(hawk, __dcCallInt64(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_oowtoint(hawk, ffi->ret_sv.ui64);
			#endif
			}
			else
			{
			#if defined(USE_DYNCALL)
				r = hawk_ooitoint(hawk, __dcCallInt64(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_ooitoint(hawk, ffi->ret_sv.i64);
			#endif
			}
			if (!r) goto hardfail;

			HAWK_STACK_SETRET (hawk, nargs, r);
			break;
		#else
			hawk_seterrbfmt (hawk, HAWK_EINVAL, "invalid return type signature - %jc", fmtc);
			goto softfail;
		#endif
		}


		case FMTC_CHAR:
		{
		#if defined(USE_DYNCALL)
			char r = dcCallChar(ffi->dc, f);
			HAWK_STACK_SETRET (hawk, nargs, HAWK_CHAR_TO_OOP(r));
		#elif defined(USE_LIBFFI)
			if (_unsigned)
			{
				HAWK_STACK_SETRET (hawk, nargs, HAWK_CHAR_TO_OOP(ffi->ret_sv.uc));
			}
			else
			{
				HAWK_STACK_SETRET (hawk, nargs, HAWK_CHAR_TO_OOP(ffi->ret_sv.c));
			}
		#endif
			break;
		}

		case FMTC_SHORT:
		{
			hawk_oop_t r;
			if (_unsigned)
			{
			#if defined(USE_DYNCALL)
				r = hawk_oowtoint(hawk, dcCallShort(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_oowtoint(hawk, ffi->ret_sv.uh);
			#endif
			}
			else
			{
			#if defined(USE_DYNCALL)
				r = hawk_ooitoint(hawk, dcCallShort(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_ooitoint(hawk, ffi->ret_sv.h);
			#endif
			}
			if (!r) goto hardfail;

			HAWK_STACK_SETRET (hawk, nargs, r);
			break;
		}

		case FMTC_INT:
		{
			hawk_oop_t r;
			if (_unsigned)
			{
			#if defined(USE_DYNCALL)
				r = hawk_oowtoint(hawk, dcCallInt(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_oowtoint(hawk, ffi->ret_sv.ui);
			#endif
			}
			else
			{
			#if defined(USE_DYNCALL)
				r = hawk_ooitoint(hawk, dcCallInt(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_ooitoint(hawk, ffi->ret_sv.i);
			#endif
			}
			if (!r) goto hardfail;
			HAWK_STACK_SETRET (hawk, nargs, r);
			break;
		}

		case FMTC_LONG:
		{
			hawk_oop_t r;
		ret_as_long:
			if (_unsigned)
			{
			#if defined(USE_DYNCALL)
				r = hawk_oowtoint(hawk, dcCallLong(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_oowtoint(hawk, ffi->ret_sv.ul);
			#endif
			}
			else
			{
			#if defined(USE_DYNCALL)
				r = hawk_ooitoint(hawk, dcCallLong(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_ooitoint(hawk, ffi->ret_sv.l);
			#endif
			}
			if (!r) goto hardfail;
			HAWK_STACK_SETRET (hawk, nargs, r);
			break;
		}

		case FMTC_LONGLONG:
		{
		#if (HAWK_SIZEOF_LONG_LONG <= HAWK_SIZEOF_LONG)
			goto ret_as_long;
		#else
			hawk_oop_t r;
			if (_unsigned)
			{
			#if defined(USE_DYNCALL)	
				r = hawk_uintmaxtoint(hawk, dcCallLongLong(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_uintmaxtoint(hawk, ffi->ret_sv.ull);
			#endif
			}
			else
			{
			#if defined(USE_DYNCALL)	
				r = hawk_intmaxtoint(hawk, dcCallLongLong(ffi->dc, f));
			#elif defined(USE_LIBFFI)
				r = hawk_intmaxtoint(hawk, ffi->ret_sv.ll);
			#endif
			}
			if (!r) goto hardfail;
			HAWK_STACK_SETRET (hawk, nargs, r);
			break;
		#endif
		}

		case FMTC_BCS:
		{
			hawk_oop_t s;
			hawk_bch_t* r;

		#if defined(USE_DYNCALL)
			r = dcCallPointer(ffi->dc, f);
		#elif defined(USE_LIBFFI)
			r = ffi->ret_sv.p;
		#endif

		#if defined(HAWK_OOCH_IS_UCH)
			s = hawk_makestringwithbchars(hawk, r, hawk_count_bcstr(r));
		#else
			s = hawk_makestring(hawk, r, hawk_count_bcstr(r));
		#endif
			if (!s) 
			{
				if (hawk->errnum == HAWK_EOOMEM) goto hardfail; /* out of object memory - hard failure*/
				goto softfail;
			}

			HAWK_STACK_SETRET (hawk, nargs, s); 
			break;
		}

		case FMTC_UCS:
		{
			hawk_oop_t s;
			hawk_uch_t* r;

		#if defined(USE_DYNCALL)
			r = dcCallPointer(ffi->dc, f);
		#elif defined(USE_LIBFFI)
			r = ffi->ret_sv.p;
		#endif

		#if defined(HAWK_OOCH_IS_UCH)
			s = hawk_makestring(hawk, r, hawk_count_ucstr(r));
		#else
			s = hawk_makestringwithuchars(hawk, r, hawk_count_ucstr(r));
		#endif
			if (!s) 
			{
				if (hawk->errnum == HAWK_EOOMEM) goto hardfail; /* out of object memory - hard failure*/
				goto softfail;
			}

			HAWK_STACK_SETRET (hawk, nargs, s); 
			break;
		}


#if 0
		case FMTC_BLOB:
		{
			/* blob as a return type isn't sufficient as it lacks the size information. 
			 * blob as an argument is just a pointer and the size can be yet another argument.
			 * it there a good way to represent this here?... TODO: */
			break;
		}
#endif

		case FMTC_POINTER:
		{
			void* r;

		#if defined(USE_DYNCALL)
			r = dcCallPointer(ffi->dc, f);
		#elif defined(USE_LIBFFI)
			r = ffi->ret_sv.p;
		#endif

			if (!HAWK_IN_SMPTR_RANGE(r)) 
			{
				/* the returned pointer is not aligned */
				goto inval;
			}

			HAWK_STACK_SETRET (hawk, nargs, HAWK_SMPTR_TO_OOP(r));
			break;
		}

		default:
		#if defined(USE_DYNCALL)
			dcCallVoid (ffi->dc, f);
		#endif
			HAWK_STACK_SETRETTORCV (hawk, nargs);
			break;
	}

	free_linked_cas (hawk, ffi);
	return HAWK_PF_SUCCESS;

noimpl:
	hawk_seterrnum (hawk, HAWK_ENOIMPL);
	goto softfail;

inval:
	hawk_seterrnum (hawk, HAWK_EINVAL);
	goto softfail;

softfail:
	free_linked_cas (hawk, ffi);
	HAWK_STACK_SETRETTOERRNUM (hawk, nargs);
	return HAWK_PF_SUCCESS;

hardfail:
	free_linked_cas (hawk, ffi);
	return HAWK_PF_HARD_FAILURE;

#else
	hawk_seterrnum (hawk, HAWK_ENOIMPL);
	HAWK_STACK_SETRETTOERRNUM (hawk, nargs);
	return HAWK_PF_SUCCESS;
#endif
}

static hawk_pfrc_t fnc_getsym (hawk_t* hawk, hawk_mod_t* mod, hawk_ooi_t nargs)
{
	ffi_t* ffi;
	hawk_oop_t name, ret;
	void* sym;

	HAWK_ASSERT (hawk, nargs == 1);

	ffi = (ffi_t*)hawk_getobjtrailer(hawk, HAWK_STACK_GETRCV(hawk, nargs), HAWK_NULL);
	name = HAWK_STACK_GETARG(hawk, nargs, 0);

	if (!HAWK_OBJ_IS_CHAR_POINTER(name))
	{
		hawk_seterrnum (hawk, HAWK_EINVAL);
		goto softfail;
	}

	if (!hawk->vmprim.dl_getsym)
	{
		hawk_seterrnum (hawk, HAWK_ENOIMPL);
		goto softfail;
	}

	sym = hawk->vmprim.dl_getsym(hawk, ffi->handle, HAWK_OBJ_GET_CHAR_SLOT(name));
	if (!sym) goto softfail;

	HAWK_DEBUG4 (hawk, "<ffi.getsym> %.*js => %p in %p\n", HAWK_OBJ_GET_SIZE(name), HAWK_OBJ_GET_CHAR_SLOT(name), sym, ffi->handle);

	ret = hawk_oowtoptr(hawk, (hawk_oow_t)sym);
	if (!ret) goto softfail;

	HAWK_STACK_SETRET (hawk, nargs, ret);
	return HAWK_PF_SUCCESS;

softfail:
	HAWK_STACK_SETRETTOERRNUM (hawk, nargs);
	return HAWK_PF_SUCCESS;
}
#endif

static int fnc_errmsg (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	ffi_list_t* ffi_list;
	hawk_val_t* retv;

	ffi_list = rtx_to_ffi_list(rtx, fi);
	retv = hawk_rtx_makestrvalwithoocstr(rtx, ffi_list->errmsg);
	if (!retv) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}
/* ------------------------------------------------------------------------ */

static hawk_mod_fnc_tab_t fnctab[] =
{
	//{ HAWK_T("call"),      { { 3, 3, HAWK_NULL },  fnc_call,     0 } },
	{ HAWK_T("close"),     { { 1, 1, HAWK_NULL },  fnc_close,     0 } },
	{ HAWK_T("errmsg"),    { { 0, 0, HAWK_NULL },  fnc_errmsg,    0 } },
	//{ HAWK_T("getsym"),    { { 1, 1, HAWK_NULL },  fnc_getsym,   0 } },
	{ HAWK_T("open"),      { { 1, 1, HAWK_NULL },  fnc_open,      0 } }
	
};

/* ------------------------------------------------------------------------ */

static int query (hawk_mod_t* mod, hawk_t* hawk, const hawk_ooch_t* name, hawk_mod_sym_t* sym)
{
	return hawk_findmodsymfnc(hawk, fnctab, HAWK_COUNTOF(fnctab), name, sym);
}

static int init (hawk_mod_t* mod, hawk_rtx_t* rtx)
{
	hawk_rbt_t* rbt;
	rtx_data_t data, * datap;
	hawk_rbt_pair_t* pair;

	rbt = (hawk_rbt_t*)mod->ctx;

	HAWK_MEMSET (&data, 0, HAWK_SIZEOF(data));
	pair = hawk_rbt_insert(rbt, &rtx, HAWK_SIZEOF(rtx), &data, HAWK_SIZEOF(data));
	if (HAWK_UNLIKELY(!pair)) return -1;

	datap = (rtx_data_t*)HAWK_RBT_VPTR(pair);
	__init_ffi_list (rtx, &datap->ffi_list);

	return 0;
}

static void fini (hawk_mod_t* mod, hawk_rtx_t* rtx)
{
	hawk_rbt_t* rbt;
	hawk_rbt_pair_t* pair;

	rbt = (hawk_rbt_t*)mod->ctx;

	/* garbage clean-up */
	pair = hawk_rbt_search(rbt, &rtx, HAWK_SIZEOF(rtx));
	if (pair)
	{
		rtx_data_t* data;

		data = (rtx_data_t*)HAWK_RBT_VPTR(pair);

		__fini_ffi_list (rtx, &data->ffi_list);

		hawk_rbt_delete (rbt, &rtx, HAWK_SIZEOF(rtx));
	}
}

static void unload (hawk_mod_t* mod, hawk_t* hawk)
{
	hawk_rbt_t* rbt;

	rbt = (hawk_rbt_t*)mod->ctx;

	HAWK_ASSERT (HAWK_RBT_SIZE(rbt) == 0);
	hawk_rbt_close (rbt);
}

int hawk_mod_ffi (hawk_mod_t* mod, hawk_t* hawk)
{
	hawk_rbt_t* rbt;

	mod->query = query;
	mod->unload = unload;
	mod->init = init;
	mod->fini = fini;

	rbt = hawk_rbt_open(hawk_getgem(hawk), 0, 1, 1);
	if (HAWK_UNLIKELY(!rbt)) return -1;

	hawk_rbt_setstyle (rbt, hawk_get_rbt_style(HAWK_RBT_STYLE_INLINE_COPIERS));
	mod->ctx = rbt;

	return 0;
}
