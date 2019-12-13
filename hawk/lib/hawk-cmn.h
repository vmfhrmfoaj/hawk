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

#ifndef _HAWK_CMN_H_
#define _HAWK_CMN_H_

/* WARNING: NEVER CHANGE/DELETE THE FOLLOWING HAWK_HAVE_CFG_H DEFINITION. 
 *          IT IS USED FOR DEPLOYMENT BY MAKEFILE.AM */
/*#define HAWK_HAVE_CFG_H*/

#if defined(HAWK_HAVE_CFG_H)
#	include <hawk-cfg.h>
#elif defined(_WIN32)
#	include <hawk-msw.h>
#elif defined(__OS2__)
#	include <hawk-os2.h>
#elif defined(__DOS__)
#	include <hawk-dos.h>
#elif defined(macintosh)
#	include <hawk-mac.h> /* class mac os */
#else
#	error UNSUPPORTED SYSTEM
#endif


/* =========================================================================
 * ARCHITECTURE/COMPILER TWEAKS
 * ========================================================================= */

#if defined(EMSCRIPTEN)
#	if defined(HAWK_SIZEOF___INT128)
#		undef HAWK_SIZEOF___INT128 
#		define HAWK_SIZEOF___INT128 0
#	endif
#	if defined(HAWK_SIZEOF_LONG) && defined(HAWK_SIZEOF_INT) && (HAWK_SIZEOF_LONG > HAWK_SIZEOF_INT)
		/* autoconf doesn't seem to match actual emscripten */
#		undef HAWK_SIZEOF_LONG
#		define HAWK_SIZEOF_LONG HAWK_SIZEOF_INT
#	endif
#	include <emscripten/emscripten.h> /* EMSCRIPTEN_KEEPALIVE */
#endif

#if defined(__GNUC__) && defined(__arm__)  && !defined(__ARM_ARCH)
#	if defined(__ARM_ARCH_8__)
#		define __ARM_ARCH 8
#	elif defined(__ARM_ARCH_7__)
#		define __ARM_ARCH 7
#	elif defined(__ARM_ARCH_6__)
#		define __ARM_ARCH 6
#	elif defined(__ARM_ARCH_5__)
#		define __ARM_ARCH 5
#	elif defined(__ARM_ARCH_4__)
#		define __ARM_ARCH 4
#	endif
#endif

/* =========================================================================
 * PRIMITIVE TYPE DEFINTIONS
 * ========================================================================= */

/* hawk_int8_t */
#if defined(HAWK_SIZEOF_CHAR) && (HAWK_SIZEOF_CHAR == 1)
#	define HAWK_HAVE_UINT8_T
#	define HAWK_HAVE_INT8_T
#	define HAWK_SIZEOF_UINT8_T (HAWK_SIZEOF_CHAR)
#	define HAWK_SIZEOF_INT8_T (HAWK_SIZEOF_CHAR)
	typedef unsigned char      hawk_uint8_t;
	typedef signed char        hawk_int8_t;
#elif defined(HAWK_SIZEOF___INT8) && (HAWK_SIZEOF___INT8 == 1)
#	define HAWK_HAVE_UINT8_T
#	define HAWK_HAVE_INT8_T
#	define HAWK_SIZEOF_UINT8_T (HAWK_SIZEOF___INT8)
#	define HAWK_SIZEOF_INT8_T (HAWK_SIZEOF___INT8)
	typedef unsigned __int8    hawk_uint8_t;
	typedef signed __int8      hawk_int8_t;
#elif defined(HAWK_SIZEOF___INT8_T) && (HAWK_SIZEOF___INT8_T == 1)
#	define HAWK_HAVE_UINT8_T
#	define HAWK_HAVE_INT8_T
#	define HAWK_SIZEOF_UINT8_T (HAWK_SIZEOF___INT8_T)
#	define HAWK_SIZEOF_INT8_T (HAWK_SIZEOF___INT8_T)
	typedef unsigned __int8_t  hawk_uint8_t;
	typedef signed __int8_t    hawk_int8_t;
#else
#	define HAWK_HAVE_UINT8_T
#	define HAWK_HAVE_INT8_T
#	define HAWK_SIZEOF_UINT8_T (1)
#	define HAWK_SIZEOF_INT8_T (1)
	typedef unsigned char      hawk_uint8_t;
	typedef signed char        hawk_int8_t;
#endif

/* hawk_int16_t */
#if defined(HAWK_SIZEOF_SHORT) && (HAWK_SIZEOF_SHORT == 2)
#	define HAWK_HAVE_UINT16_T
#	define HAWK_HAVE_INT16_T
#	define HAWK_SIZEOF_UINT16_T (HAWK_SIZEOF_SHORT)
#	define HAWK_SIZEOF_INT16_T (HAWK_SIZEOF_SHORT)
	typedef unsigned short int  hawk_uint16_t;
	typedef signed short int    hawk_int16_t;
#elif defined(HAWK_SIZEOF___INT16) && (HAWK_SIZEOF___INT16 == 2)
#	define HAWK_HAVE_UINT16_T
#	define HAWK_HAVE_INT16_T
#	define HAWK_SIZEOF_UINT16_T (HAWK_SIZEOF___INT16)
#	define HAWK_SIZEOF_INT16_T (HAWK_SIZEOF___INT16)
	typedef unsigned __int16    hawk_uint16_t;
	typedef signed __int16      hawk_int16_t;
#elif defined(HAWK_SIZEOF___INT16_T) && (HAWK_SIZEOF___INT16_T == 2)
#	define HAWK_HAVE_UINT16_T
#	define HAWK_HAVE_INT16_T
#	define HAWK_SIZEOF_UINT16_T (HAWK_SIZEOF___INT16_T)
#	define HAWK_SIZEOF_INT16_T (HAWK_SIZEOF___INT16_T)
	typedef unsigned __int16_t  hawk_uint16_t;
	typedef signed __int16_t    hawk_int16_t;
#else
#	define HAWK_HAVE_UINT16_T
#	define HAWK_HAVE_INT16_T
#	define HAWK_SIZEOF_UINT16_T (2)
#	define HAWK_SIZEOF_INT16_T (2)
	typedef unsigned short int  hawk_uint16_t;
	typedef signed short int    hawk_int16_t;
#endif


/* hawk_int32_t */
#if defined(HAWK_SIZEOF_INT) && (HAWK_SIZEOF_INT == 4)
#	define HAWK_HAVE_UINT32_T
#	define HAWK_HAVE_INT32_T
#	define HAWK_SIZEOF_UINT32_T (HAWK_SIZEOF_INT)
#	define HAWK_SIZEOF_INT32_T (HAWK_SIZEOF_INT)
	typedef unsigned int        hawk_uint32_t;
	typedef signed int          hawk_int32_t;
#elif defined(HAWK_SIZEOF_LONG) && (HAWK_SIZEOF_LONG == 4)
#	define HAWK_HAVE_UINT32_T
#	define HAWK_HAVE_INT32_T
#	define HAWK_SIZEOF_UINT32_T (HAWK_SIZEOF_LONG)
#	define HAWK_SIZEOF_INT32_T (HAWK_SIZEOF_LONG)
	typedef unsigned long int   hawk_uint32_t;
	typedef signed long int     hawk_int32_t;
#elif defined(HAWK_SIZEOF___INT32) && (HAWK_SIZEOF___INT32 == 4)
#	define HAWK_HAVE_UINT32_T
#	define HAWK_HAVE_INT32_T
#	define HAWK_SIZEOF_UINT32_T (HAWK_SIZEOF___INT32)
#	define HAWK_SIZEOF_INT32_T (HAWK_SIZEOF___INT32)
	typedef unsigned __int32    hawk_uint32_t;
	typedef signed __int32      hawk_int32_t;
#elif defined(HAWK_SIZEOF___INT32_T) && (HAWK_SIZEOF___INT32_T == 4)
#	define HAWK_HAVE_UINT32_T
#	define HAWK_HAVE_INT32_T
#	define HAWK_SIZEOF_UINT32_T (HAWK_SIZEOF___INT32_T)
#	define HAWK_SIZEOF_INT32_T (HAWK_SIZEOF___INT32_T)
	typedef unsigned __int32_t  hawk_uint32_t;
	typedef signed __int32_t    hawk_int32_t;
#elif defined(__DOS__)
#	define HAWK_HAVE_UINT32_T
#	define HAWK_HAVE_INT32_T
#	define HAWK_SIZEOF_UINT32_T (4)
#	define HAWK_SIZEOF_INT32_T (4)
	typedef unsigned long int   hawk_uint32_t;
	typedef signed long int     hawk_int32_t;
#else
#	define HAWK_HAVE_UINT32_T
#	define HAWK_HAVE_INT32_T
#	define HAWK_SIZEOF_UINT32_T (4)
#	define HAWK_SIZEOF_INT32_T (4)
	typedef unsigned int        hawk_uint32_t;
	typedef signed int          hawk_int32_t;
#endif

/* hawk_int64_t */
#if defined(HAWK_SIZEOF_INT) && (HAWK_SIZEOF_INT == 8)
#	define HAWK_HAVE_UINT64_T
#	define HAWK_HAVE_INT64_T
#	define HAWK_SIZEOF_UINT64_T (HAWK_SIZEOF_INT)
#	define HAWK_SIZEOF_INT64_T (HAWK_SIZEOF_INT)
	typedef unsigned int        hawk_uint64_t;
	typedef signed int          hawk_int64_t;
#elif defined(HAWK_SIZEOF_LONG) && (HAWK_SIZEOF_LONG == 8)
#	define HAWK_HAVE_UINT64_T
#	define HAWK_HAVE_INT64_T
#	define HAWK_SIZEOF_UINT64_T (HAWK_SIZEOF_LONG)
#	define HAWK_SIZEOF_INT64_T (HAWK_SIZEOF_LONG)
	typedef unsigned long int  hawk_uint64_t;
	typedef signed long int    hawk_int64_t;
#elif defined(HAWK_SIZEOF_LONG_LONG) && (HAWK_SIZEOF_LONG_LONG == 8)
#	define HAWK_HAVE_UINT64_T
#	define HAWK_HAVE_INT64_T
#	define HAWK_SIZEOF_UINT64_T (HAWK_SIZEOF_LONG_LONG)
#	define HAWK_SIZEOF_INT64_T (HAWK_SIZEOF_LONG_LONG)
	typedef unsigned long long int  hawk_uint64_t;
	typedef signed long long int    hawk_int64_t;
#elif defined(HAWK_SIZEOF___INT64) && (HAWK_SIZEOF___INT64 == 8)
#	define HAWK_HAVE_UINT64_T
#	define HAWK_HAVE_INT64_T
#	define HAWK_SIZEOF_UINT64_T (HAWK_SIZEOF_LONG___INT64)
#	define HAWK_SIZEOF_INT64_T (HAWK_SIZEOF_LONG___INT64)
	typedef unsigned __int64    hawk_uint64_t;
	typedef signed __int64      hawk_int64_t;
#elif defined(HAWK_SIZEOF___INT64_T) && (HAWK_SIZEOF___INT64_T == 8)
#	define HAWK_HAVE_UINT64_T
#	define HAWK_HAVE_INT64_T
#	define HAWK_SIZEOF_UINT64_T (HAWK_SIZEOF_LONG___INT64_T)
#	define HAWK_SIZEOF_INT64_T (HAWK_SIZEOF_LONG___INT64_T)
	typedef unsigned __int64_t  hawk_uint64_t;
	typedef signed __int64_t    hawk_int64_t;
#else
	/* no 64-bit integer */
#endif

/* hawk_int128_t */
#if defined(HAWK_SIZEOF_INT) && (HAWK_SIZEOF_INT == 16)
#	define HAWK_HAVE_UINT128_T
#	define HAWK_HAVE_INT128_T
#	define HAWK_SIZEOF_UINT128_T (HAWK_SIZEOF_INT)
#	define HAWK_SIZEOF_INT128_T (HAWK_SIZEOF_INT)
	typedef unsigned int        hawk_uint128_t;
	typedef signed int          hawk_int128_t;
#elif defined(HAWK_SIZEOF_LONG) && (HAWK_SIZEOF_LONG == 16)
#	define HAWK_HAVE_UINT128_T
#	define HAWK_HAVE_INT128_T
#	define HAWK_SIZEOF_UINT128_T (HAWK_SIZEOF_LONG)
#	define HAWK_SIZEOF_INT128_T (HAWK_SIZEOF_LONG)
	typedef unsigned long int   hawk_uint128_t;
	typedef signed long int     hawk_int128_t;
#elif defined(HAWK_SIZEOF_LONG_LONG) && (HAWK_SIZEOF_LONG_LONG == 16)
#	define HAWK_HAVE_UINT128_T
#	define HAWK_HAVE_INT128_T
#	define HAWK_SIZEOF_UINT128_T (HAWK_SIZEOF_LONG_LONG)
#	define HAWK_SIZEOF_INT128_T (HAWK_SIZEOF_LONG_LONG)
	typedef unsigned long long int hawk_uint128_t;
	typedef signed long long int   hawk_int128_t;
#elif defined(HAWK_SIZEOF___INT128) && (HAWK_SIZEOF___INT128 == 16)
#	define HAWK_HAVE_UINT128_T
#	define HAWK_HAVE_INT128_T
#	define HAWK_SIZEOF_UINT128_T (HAWK_SIZEOF___INT128)
#	define HAWK_SIZEOF_INT128_T (HAWK_SIZEOF___INT128)
	typedef unsigned __int128    hawk_uint128_t;
	typedef signed __int128      hawk_int128_t;
#elif defined(HAWK_SIZEOF___INT128_T) && (HAWK_SIZEOF___INT128_T == 16)
#	define HAWK_HAVE_UINT128_T
#	define HAWK_HAVE_INT128_T
#	define HAWK_SIZEOF_UINT128_T (HAWK_SIZEOF___INT128_T)
#	define HAWK_SIZEOF_INT128_T (HAWK_SIZEOF___INT128_T)
	#if defined(HAWK_SIZEOF___UINT128_T) && (HAWK_SIZEOF___UINT128_T == HAWK_SIZEOF___INT128_T)
	typedef __uint128_t  hawk_uint128_t;
	typedef __int128_t   hawk_int128_t;
	#elif defined(__clang__)
	typedef __uint128_t  hawk_uint128_t;
	typedef __int128_t   hawk_int128_t;
	#else
	typedef unsigned __int128_t  hawk_uint128_t;
	typedef signed __int128_t    hawk_int128_t;
	#endif
#else
	/* no 128-bit integer */
#endif

#if defined(HAWK_HAVE_UINT8_T) && (HAWK_SIZEOF_VOID_P == 1)
#	error UNSUPPORTED POINTER SIZE
#elif defined(HAWK_HAVE_UINT16_T) && (HAWK_SIZEOF_VOID_P == 2)
	typedef hawk_uint16_t hawk_uintptr_t;
	typedef hawk_int16_t hawk_intptr_t;
	typedef hawk_uint8_t hawk_ushortptr_t;
	typedef hawk_int8_t hawk_shortptr_t;
#elif defined(HAWK_HAVE_UINT32_T) && (HAWK_SIZEOF_VOID_P == 4)
	typedef hawk_uint32_t hawk_uintptr_t;
	typedef hawk_int32_t hawk_intptr_t;
	typedef hawk_uint16_t hawk_ushortptr_t;
	typedef hawk_int16_t hawk_shortptr_t;
#elif defined(HAWK_HAVE_UINT64_T) && (HAWK_SIZEOF_VOID_P == 8)
	typedef hawk_uint64_t hawk_uintptr_t;
	typedef hawk_int64_t hawk_intptr_t;
	typedef hawk_uint32_t hawk_ushortptr_t;
	typedef hawk_int32_t hawk_shortptr_t;
#elif defined(HAWK_HAVE_UINT128_T) && (HAWK_SIZEOF_VOID_P == 16) 
	typedef hawk_uint128_t hawk_uintptr_t;
	typedef hawk_int128_t hawk_intptr_t;
	typedef hawk_uint64_t hawk_ushortptr_t;
	typedef hawk_int64_t hawk_shortptr_t;
#else
#	error UNKNOWN POINTER SIZE
#endif

#define HAWK_SIZEOF_INTPTR_T HAWK_SIZEOF_VOID_P
#define HAWK_SIZEOF_UINTPTR_T HAWK_SIZEOF_VOID_P
#define HAWK_SIZEOF_SHORTPTR_T (HAWK_SIZEOF_VOID_P / 2)
#define HAWK_SIZEOF_USHORTPTR_T (HAWK_SIZEOF_VOID_P / 2)

#if defined(HAWK_HAVE_INT128_T)
#	define HAWK_SIZEOF_INTMAX_T 16
#	define HAWK_SIZEOF_UINTMAX_T 16
	typedef hawk_int128_t hawk_intmax_t;
	typedef hawk_uint128_t hawk_uintmax_t;
#elif defined(HAWK_HAVE_INT64_T)
#	define HAWK_SIZEOF_INTMAX_T 8
#	define HAWK_SIZEOF_UINTMAX_T 8
	typedef hawk_int64_t hawk_intmax_t;
	typedef hawk_uint64_t hawk_uintmax_t;
#elif defined(HAWK_HAVE_INT32_T)
#	define HAWK_SIZEOF_INTMAX_T 4
#	define HAWK_SIZEOF_UINTMAX_T 4
	typedef hawk_int32_t hawk_intmax_t;
	typedef hawk_uint32_t hawk_uintmax_t;
#elif defined(HAWK_HAVE_INT16_T)
#	define HAWK_SIZEOF_INTMAX_T 2
#	define HAWK_SIZEOF_UINTMAX_T 2
	typedef hawk_int16_t hawk_intmax_t;
	typedef hawk_uint16_t hawk_uintmax_t;
#elif defined(HAWK_HAVE_INT8_T)
#	define HAWK_SIZEOF_INTMAX_T 1
#	define HAWK_SIZEOF_UINTMAX_T 1
	typedef hawk_int8_t hawk_intmax_t;
	typedef hawk_uint8_t hawk_uintmax_t;
#else
#	error UNKNOWN INTMAX SIZE
#endif

#if defined(HAWK_USE_INTMAX)
typedef hawk_intmax_t hawk_int_t;
typedef hawk_uintmax_t hawk_uint_t;
#define HAWK_SIZEOF_INT_T HAWK_SIZEZOF_INTMAX_T
#else
typedef hawk_intptr_t hawk_int_t;
typedef hawk_uintptr_t hawk_uint_t;
#define HAWK_SIZEOF_INT_T HAWK_SIZEZOF_INTPTR_T
#endif

/* =========================================================================
 * FLOATING-POINT TYPE
 * ========================================================================= */
/** \typedef hawk_fltbas_t
 * The hawk_fltbas_t type defines the largest floating-pointer number type
 * naturally supported.
 */
#if defined(__FreeBSD__) || defined(__MINGW32__)
	/* TODO: check if the support for long double is complete.
	 *       if so, use long double for hawk_flt_t */
	typedef double hawk_fltbas_t;
#	define HAWK_SIZEOF_FLTBAS_T HAWK_SIZEOF_DOUBLE
#elif HAWK_SIZEOF_LONG_DOUBLE > HAWK_SIZEOF_DOUBLE
	typedef long double hawk_fltbas_t;
#	define HAWK_SIZEOF_FLTBAS_T HAWK_SIZEOF_LONG_DOUBLE
#else
	typedef double hawk_fltbas_t;
#	define HAWK_SIZEOF_FLTBAS_T HAWK_SIZEOF_DOUBLE
#endif

/** \typedef hawk_fltmax_t
 * The hawk_fltmax_t type defines the largest floating-pointer number type
 * ever supported.
 */
#if HAWK_SIZEOF___FLOAT128 >= HAWK_SIZEOF_FLTBAS_T
	/* the size of long double may be equal to the size of __float128
	 * for alignment on some platforms */
	typedef __float128 hawk_fltmax_t;
#	define HAWK_SIZEOF_FLTMAX_T HAWK_SIZEOF___FLOAT128
#	define HAWK_FLTMAX_REQUIRE_QUADMATH 1
#else
	typedef hawk_fltbas_t hawk_fltmax_t;
#	define HAWK_SIZEOF_FLTMAX_T HAWK_SIZEOF_FLTBAS_T
#	undef HAWK_FLTMAX_REQUIRE_QUADMATH
#endif


#if defined(HAWK_USE_FLTMAX)
typedef hawk_fltmax_t hawk_flt_t;
#define HAWK_SIZEOF_FLT_T HAWK_SIZEOF_FLTMAX_T
#else
typedef hawk_fltbas_t hawk_flt_t;
#define HAWK_SIZEOF_FLT_T HAWK_SIZEOF_FLTBAS_T
#endif


/* =========================================================================
 * BASIC HARD-CODED DEFINES
 * ========================================================================= */
#define HAWK_BITS_PER_BYTE (8)
/* the maximum number of bch charaters to represent a single uch character */
#define HAWK_BCSIZE_MAX 6

/* =========================================================================
 * BASIC HAWK TYPES
 * ========================================================================= */

typedef char                    hawk_bch_t;
typedef int                     hawk_bci_t;
typedef unsigned int            hawk_bcu_t;
typedef unsigned char           hawk_bchu_t; /* unsigned version of hawk_bch_t for inner working */
#define HAWK_SIZEOF_BCH_T HAWK_SIZEOF_CHAR
#define HAWK_SIZEOF_BCI_T HAWK_SIZEOF_INT

#if defined(HAWK_UNICODE_SIZE) && (HAWK_UNICODE_SIZE >= 4)
#	if defined(__GNUC__) && defined(__CHAR32_TYPE__)
	typedef __CHAR32_TYPE__    hawk_uch_t;
#	else
	typedef hawk_uint32_t      hawk_uch_t;
#	endif
	typedef hawk_uint32_t      hawk_uchu_t; /* same as hawk_uch_t as it is already unsigned */
#	define HAWK_SIZEOF_UCH_T 4

#elif defined(__GNUC__) && defined(__CHAR16_TYPE__)
	typedef __CHAR16_TYPE__    hawk_uch_t; 
	typedef hawk_uint16_t      hawk_uchu_t; /* same as hawk_uch_t as it is already unsigned */
#	define HAWK_SIZEOF_UCH_T 2
#	define HAWK_UCH_IS_CHAR16_T
#else
	typedef hawk_uint16_t      hawk_uch_t;
	typedef hawk_uint16_t      hawk_uchu_t; /* same as hawk_uch_t as it is already unsigned */
#	define HAWK_SIZEOF_UCH_T 2
#endif

typedef hawk_int32_t             hawk_uci_t;
typedef hawk_uint32_t            hawk_ucu_t;
#define HAWK_SIZEOF_UCI_T 4

typedef hawk_uint8_t             hawk_oob_t;

/* [NOTE] sizeof(hawk_oop_t) must be equal to sizeof(hawk_oow_t) */
typedef hawk_uintptr_t           hawk_oow_t;
typedef hawk_intptr_t            hawk_ooi_t;
#define HAWK_SIZEOF_OOW_T HAWK_SIZEOF_UINTPTR_T
#define HAWK_SIZEOF_OOI_T HAWK_SIZEOF_INTPTR_T
#define HAWK_OOW_BITS  (HAWK_SIZEOF_OOW_T * HAWK_BITS_PER_BYTE)
#define HAWK_OOI_BITS  (HAWK_SIZEOF_OOI_T * HAWK_BITS_PER_BYTE)

typedef hawk_ushortptr_t         hawk_oohw_t; /* half word - half word */
typedef hawk_shortptr_t          hawk_oohi_t; /* signed half word */
#define HAWK_SIZEOF_OOHW_T HAWK_SIZEOF_USHORTPTR_T
#define HAWK_SIZEOF_OOHI_T HAWK_SIZEOF_SHORTPTR_T
#define HAWK_OOHW_BITS  (HAWK_SIZEOF_OOHW_T * HAWK_BITS_PER_BYTE)
#define HAWK_OOHI_BITS  (HAWK_SIZEOF_OOHI_T * HAWK_BITS_PER_BYTE)

typedef struct hawk_ucs_t hawk_ucs_t;
struct hawk_ucs_t
{
	hawk_uch_t* ptr;
	hawk_oow_t  len;
};

typedef struct hawk_bcs_t hawk_bcs_t;
struct hawk_bcs_t
{
	hawk_bch_t* ptr;
	hawk_oow_t  len;
};

#if defined(HAWK_ENABLE_UNICODE)
	typedef hawk_uch_t               hawk_ooch_t;
	typedef hawk_uchu_t              hawk_oochu_t;
	typedef hawk_uci_t               hawk_ooci_t;
	typedef hawk_ucu_t               hawk_oocu_t;
	typedef hawk_ucs_t               hawk_oocs_t;
#	define HAWK_OOCH_IS_UCH
#	define HAWK_SIZEOF_OOCH_T HAWK_SIZEOF_UCH_T
#else
	typedef hawk_bch_t               hawk_ooch_t;
	typedef hawk_bchu_t              hawk_oochu_t;
	typedef hawk_bci_t               hawk_ooci_t;
	typedef hawk_bcu_t               hawk_oocu_t;
	typedef hawk_bcs_t               hawk_oocs_t;
#	define HAWK_OOCH_IS_BCH
#	define HAWK_SIZEOF_OOCH_T HAWK_SIZEOF_BCH_T
#endif

typedef unsigned int hawk_bitmask_t;


typedef struct hawk_ptl_t hawk_ptl_t;
struct hawk_ptl_t
{
	void*      ptr;
	hawk_oow_t len;
};

typedef struct hawk_link_t hawk_link_t;
struct hawk_link_t
{
	hawk_link_t* link;
};

/* =========================================================================
 * TIME-RELATED TYPES
 * =========================================================================*/
#define HAWK_MSECS_PER_SEC  (1000)
#define HAWK_MSECS_PER_MIN  (HAWK_MSECS_PER_SEC * HAWK_SECS_PER_MIN)
#define HAWK_MSECS_PER_HOUR (HAWK_MSECS_PER_SEC * HAWK_SECS_PER_HOUR)
#define HAWK_MSECS_PER_DAY  (HAWK_MSECS_PER_SEC * HAWK_SECS_PER_DAY)

#define HAWK_USECS_PER_MSEC (1000)
#define HAWK_NSECS_PER_USEC (1000)
#define HAWK_NSECS_PER_MSEC (HAWK_NSECS_PER_USEC * HAWK_USECS_PER_MSEC)
#define HAWK_USECS_PER_SEC  (HAWK_USECS_PER_MSEC * HAWK_MSECS_PER_SEC)
#define HAWK_NSECS_PER_SEC  (HAWK_NSECS_PER_USEC * HAWK_USECS_PER_MSEC * HAWK_MSECS_PER_SEC)

#define HAWK_SECNSEC_TO_MSEC(sec,nsec) \
        (((hawk_intptr_t)(sec) * HAWK_MSECS_PER_SEC) + ((hawk_intptr_t)(nsec) / HAWK_NSECS_PER_MSEC))

#define HAWK_SECNSEC_TO_USEC(sec,nsec) \
        (((hawk_intptr_t)(sec) * HAWK_USECS_PER_SEC) + ((hawk_intptr_t)(nsec) / HAWK_NSECS_PER_USEC))

#define HAWK_SECNSEC_TO_NSEC(sec,nsec) \
        (((hawk_intptr_t)(sec) * HAWK_NSECS_PER_SEC) + (hawk_intptr_t)(nsec))

#define HAWK_SEC_TO_MSEC(sec) ((sec) * HAWK_MSECS_PER_SEC)
#define HAWK_MSEC_TO_SEC(sec) ((sec) / HAWK_MSECS_PER_SEC)

#define HAWK_USEC_TO_NSEC(usec) ((usec) * HAWK_NSECS_PER_USEC)
#define HAWK_NSEC_TO_USEC(nsec) ((nsec) / HAWK_NSECS_PER_USEC)

#define HAWK_MSEC_TO_NSEC(msec) ((msec) * HAWK_NSECS_PER_MSEC)
#define HAWK_NSEC_TO_MSEC(nsec) ((nsec) / HAWK_NSECS_PER_MSEC)

#define HAWK_MSEC_TO_USEC(msec) ((msec) * HAWK_USECS_PER_MSEC)
#define HAWK_USEC_TO_MSEC(usec) ((usec) / HAWK_USECS_PER_MSEC)

#define HAWK_SEC_TO_NSEC(sec) ((sec) * HAWK_NSECS_PER_SEC)
#define HAWK_NSEC_TO_SEC(nsec) ((nsec) / HAWK_NSECS_PER_SEC)

#define HAWK_SEC_TO_USEC(sec) ((sec) * HAWK_USECS_PER_SEC)
#define HAWK_USEC_TO_SEC(usec) ((usec) / HAWK_USECS_PER_SEC)

typedef struct hawk_ntime_t hawk_ntime_t;
struct hawk_ntime_t
{
	hawk_intptr_t  sec;
	hawk_int32_t   nsec; /* nanoseconds */
};

#define HAWK_INIT_NTIME(c,s,ns) (((c)->sec = (s)), ((c)->nsec = (ns)))
#define HAWK_CLEAR_NTIME(c) HAWK_INIT_NTIME(c, 0, 0)

#define HAWK_ADD_NTIME(c,a,b) \
	do { \
		(c)->sec = (a)->sec + (b)->sec; \
		(c)->nsec = (a)->nsec + (b)->nsec; \
		while ((c)->nsec >= HAWK_NSECS_PER_SEC) { (c)->sec++; (c)->nsec -= HAWK_NSECS_PER_SEC; } \
	} while(0)

#define HAWK_ADD_NTIME_SNS(c,a,s,ns) \
	do { \
		(c)->sec = (a)->sec + (s); \
		(c)->nsec = (a)->nsec + (ns); \
		while ((c)->nsec >= HAWK_NSECS_PER_SEC) { (c)->sec++; (c)->nsec -= HAWK_NSECS_PER_SEC; } \
	} while(0)

#define HAWK_SUB_NTIME(c,a,b) \
	do { \
		(c)->sec = (a)->sec - (b)->sec; \
		(c)->nsec = (a)->nsec - (b)->nsec; \
		while ((c)->nsec < 0) { (c)->sec--; (c)->nsec += HAWK_NSECS_PER_SEC; } \
	} while(0)

#define HAWK_SUB_NTIME_SNS(c,a,s,ns) \
	do { \
		(c)->sec = (a)->sec - s; \
		(c)->nsec = (a)->nsec - ns; \
		while ((c)->nsec < 0) { (c)->sec--; (c)->nsec += HAWK_NSECS_PER_SEC; } \
	} while(0)


#define HAWK_CMP_NTIME(a,b) (((a)->sec == (b)->sec)? ((a)->nsec - (b)->nsec): ((a)->sec - (b)->sec))

/* if time has been normalized properly, nsec must be equal to or
 * greater than 0. */
#define HAWK_IS_NEG_NTIME(x) ((x)->sec < 0)
#define HAWK_IS_POS_NTIME(x) ((x)->sec > 0 || ((x)->sec == 0 && (x)->nsec > 0))
#define HAWK_IS_ZERO_NTIME(x) ((x)->sec == 0 && (x)->nsec == 0)

/* =========================================================================
 * PRIMITIVE MACROS
 * ========================================================================= */
#define HAWK_UCI_EOF ((hawk_uci_t)-1)
#define HAWK_BCI_EOF ((hawk_bci_t)-1)
#define HAWK_OOCI_EOF ((hawk_ooci_t)-1)

#define HAWK_SIZEOF(x) (sizeof(x))
#define HAWK_COUNTOF(x) (sizeof(x) / sizeof((x)[0]))
#define HAWK_BITSOF(x) (sizeof(x) * HAWK_BITS_PER_BYTE)

/**
 * The HAWK_OFFSETOF() macro returns the offset of a field from the beginning
 * of a structure.
 */
#define HAWK_OFFSETOF(type,member) ((hawk_uintptr_t)&((type*)0)->member)

/**
 * The HAWK_ALIGNOF() macro returns the alignment size of a structure.
 * Note that this macro may not work reliably depending on the type given.
 */
#define HAWK_ALIGNOF(type) HAWK_OFFSETOF(struct { hawk_uint8_t d1; type d2; }, d2)
        /*(sizeof(struct { hawk_uint8_t d1; type d2; }) - sizeof(type))*/

#if defined(__cplusplus)
#	if (__cplusplus >= 201103L) /* C++11 */
#		define HAWK_NULL nullptr
#	else
#		define HAWK_NULL (0)
#	endif
#else
#	define HAWK_NULL ((void*)0)
#endif

/* make a bit mask that can mask off low n bits */
#define HAWK_LBMASK(type,n) (~(~((type)0) << (n))) 
#define HAWK_LBMASK_SAFE(type,n) (((n) < HAWK_BITSOF(type))? HAWK_LBMASK(type,n): ~(type)0)

/* make a bit mask that can mask off hig n bits */
#define HAWK_HBMASK(type,n) (~(~((type)0) >> (n)))
#define HAWK_HBMASK_SAFE(type,n) (((n) < HAWK_BITSOF(type))? HAWK_HBMASK(type,n): ~(type)0)

/* get 'length' bits starting from the bit at the 'offset' */
#define HAWK_GETBITS(type,value,offset,length) \
	((((type)(value)) >> (offset)) & HAWK_LBMASK(type,length))

#define HAWK_CLEARBITS(type,value,offset,length) \
	(((type)(value)) & ~(HAWK_LBMASK(type,length) << (offset)))

#define HAWK_SETBITS(type,value,offset,length,bits) \
	(value = (HAWK_CLEARBITS(type,value,offset,length) | (((bits) & HAWK_LBMASK(type,length)) << (offset))))

#define HAWK_FLIPBITS(type,value,offset,length) \
	(((type)(value)) ^ (HAWK_LBMASK(type,length) << (offset)))

#define HAWK_ORBITS(type,value,offset,length,bits) \
	(value = (((type)(value)) | (((bits) & HAWK_LBMASK(type,length)) << (offset))))


/** 
 * The HAWK_BITS_MAX() macros calculates the maximum value that the 'nbits'
 * bits of an unsigned integer of the given 'type' can hold.
 * \code
 * printf ("%u", HAWK_BITS_MAX(unsigned int, 5));
 * \endcode
 */
/*#define HAWK_BITS_MAX(type,nbits) ((((type)1) << (nbits)) - 1)*/
#define HAWK_BITS_MAX(type,nbits) ((~(type)0) >> (HAWK_BITSOF(type) - (nbits)))

/* =========================================================================
 * MMGR
 * ========================================================================= */
typedef struct hawk_mmgr_t hawk_mmgr_t;

/** 
 * allocate a memory chunk of the size \a n.
 * \return pointer to a memory chunk on success, #HAWK_NULL on failure.
 */
typedef void* (*hawk_mmgr_alloc_t)   (hawk_mmgr_t* mmgr, hawk_oow_t n);
/** 
 * resize a memory chunk pointed to by \a ptr to the size \a n.
 * \return pointer to a memory chunk on success, #HAWK_NULL on failure.
 */
typedef void* (*hawk_mmgr_realloc_t) (hawk_mmgr_t* mmgr, void* ptr, hawk_oow_t n);
/**
 * free a memory chunk pointed to by \a ptr.
 */
typedef void  (*hawk_mmgr_free_t)    (hawk_mmgr_t* mmgr, void* ptr);

/**
 * The hawk_mmgr_t type defines the memory management interface.
 * As the type is merely a structure, it is just used as a single container
 * for memory management functions with a pointer to user-defined data. 
 * The user-defined data pointer \a ctx is passed to each memory management 
 * function whenever it is called. You can allocate, reallocate, and free 
 * a memory chunk.
 *
 * For example, a hawk_xxx_open() function accepts a pointer of the hawk_mmgr_t
 * type and the xxx object uses it to manage dynamic data within the object. 
 */
struct hawk_mmgr_t
{
	hawk_mmgr_alloc_t   alloc;   /**< allocation function */
	hawk_mmgr_realloc_t realloc; /**< resizing function */
	hawk_mmgr_free_t    free;    /**< disposal function */
	void*               ctx;     /**< user-defined data pointer */
};

/**
 * The HAWK_MMGR_ALLOC() macro allocates a memory block of the \a size bytes
 * using the \a mmgr memory manager.
 */
#define HAWK_MMGR_ALLOC(mmgr,size) ((mmgr)->alloc(mmgr,size))

/**
 * The HAWK_MMGR_REALLOC() macro resizes a memory block pointed to by \a ptr 
 * to the \a size bytes using the \a mmgr memory manager.
 */
#define HAWK_MMGR_REALLOC(mmgr,ptr,size) ((mmgr)->realloc(mmgr,ptr,size))

/** 
 * The HAWK_MMGR_FREE() macro deallocates the memory block pointed to by \a ptr.
 */
#define HAWK_MMGR_FREE(mmgr,ptr) ((mmgr)->free(mmgr,ptr))


/* =========================================================================
 * CMGR
 * =========================================================================*/

typedef struct hawk_cmgr_t hawk_cmgr_t;

typedef hawk_oow_t (*hawk_cmgr_bctouc_t) (
	const hawk_bch_t*   mb, 
	hawk_oow_t         size,
	hawk_uch_t*         wc
);

typedef hawk_oow_t (*hawk_cmgr_uctobc_t) (
	hawk_uch_t    wc,
	hawk_bch_t*   mb,
	hawk_oow_t   size
);

/**
 * The hawk_cmgr_t type defines the character-level interface to 
 * multibyte/wide-character conversion. This interface doesn't 
 * provide any facility to store conversion state in a context
 * independent manner. This leads to the limitation that it can
 * handle a stateless multibyte encoding only.
 */
struct hawk_cmgr_t
{
	hawk_cmgr_bctouc_t bctouc;
	hawk_cmgr_uctobc_t uctobc;
};

/* =========================================================================
 * FORWARD DECLARATION FOR MAIN HAWK STRUCTURE
 * =========================================================================*/
typedef struct hawk_t hawk_t;
typedef struct hawk_val_t hawk_val_t;
typedef struct hawk_gem_t hawk_gem_t;

#define HAWK_ERRMSG_CAPA 2048

/**
 * The hawk_errnum_t type defines error codes.
 */
enum hawk_errnum_t
{
	HAWK_ENOERR,  /**< no error */
	HAWK_EOTHER,  /**< other error */
	HAWK_ENOIMPL, /**< not implemented */
	HAWK_ESYSERR, /**< subsystem error */
	HAWK_EINTERN, /**< internal error */

	/* common errors */
	HAWK_ENOMEM,  /**< insufficient memory */
	HAWK_EINVAL,  /**< invalid parameter or data */
	HAWK_EACCES,  /**< access denied */
	HAWK_EPERM,   /**< operation not permitted */
	HAWK_ENOSUP,  /**< not supported */
	HAWK_ENOENT,  /**< '${0}' not found */
	HAWK_EEXIST,  /**< '${0}' already exists */
	HAWK_EIOERR,  /**< I/O error */
	HAWK_EBUFFULL, /**< buffer full  */
	HAWK_EECERR,  /**< encoding conversion error */

	/* mostly parse errors */
	HAWK_EOPEN,   /**< cannot open '${0}' */
	HAWK_EREAD,   /**< cannot read '${0}' */
	HAWK_EWRITE,  /**< cannot write '${0}' */
	HAWK_ECLOSE,  /**< cannot close '${0}' */

	HAWK_EBLKNST, /**< block nested too deeply */
	HAWK_EEXPRNST,/**< expression nested too deeply */

	HAWK_ELXCHR,  /**< invalid character '${0}' */
	HAWK_ELXDIG,  /**< invalid digit '${0}' */

	HAWK_EEOF,    /**< unexpected end of source */
	HAWK_ECMTNC,  /**< comment not closed properly */
	HAWK_ESTRNC,  /**< string or regular expression not closed */
	HAWK_EMBSCHR, /**< invalid mbs character '%{0}' */
	HAWK_ELBRACE, /**< left brace expected in place of '${0}' */
	HAWK_ELPAREN, /**< left parenthesis expected in place of '${0}' */
	HAWK_ERPAREN, /**< right parenthesis expected in place of '${0}' */
	HAWK_ERBRACK, /**< right bracket expected in place of '${0}' */
	HAWK_ECOMMA,  /**< comma expected in place of '${0}' */
	HAWK_ESCOLON, /**< semicolon expected in place of '${0}' */
	HAWK_ECOLON,  /**< colon expected in place of '${0}' */
	HAWK_EINTLIT, /**< integer literal expected in place of '${0}' */
	HAWK_ESTMEND, /**< statement not ending with a semicolon */
	HAWK_EKWIN,   /**< keyword 'in' expected in place of '${0}' */
	HAWK_ENOTVAR, /**< right-hand side of 'in' not a variable */
	HAWK_EEXPRNR, /**< expression not recognized around '${0}' */

	HAWK_EKWFNC,    /**< keyword 'function' expected in place of '${0}' */
	HAWK_EKWWHL,    /**< keyword 'while' expected in place of '${0}' */
	HAWK_EASSIGN,   /**< assignment statement expected */
	HAWK_EIDENT,    /**< identifier expected in place of '${0}' */
	HAWK_EFUNNAM,   /**< '${0}' not a valid function name */
	HAWK_EBLKBEG,   /**< BEGIN not followed by left bracket on the same line */
	HAWK_EBLKEND,   /**< END not followed by left bracket on the same line */
	HAWK_EKWRED,    /**< keyword '${0}' redefined */
	HAWK_EFNCRED,   /**< intrinsic function '${0}' redefined */
	HAWK_EFUNRED,   /**< function '${0}' redefined */
	HAWK_EGBLRED,   /**< global variable '${0}' redefined */
	HAWK_EPARRED,   /**< parameter '${0}' redefined */
	HAWK_EVARRED,   /**< variable '${0}' redefined */
	HAWK_EDUPPAR,   /**< duplicate parameter name '${0}' */
	HAWK_EDUPGBL,   /**< duplicate global variable name '${0}' */
	HAWK_EDUPLCL,   /**< duplicate local variable name '${0}' */
	HAWK_EBADPAR,   /**< '${0}' not a valid parameter name */
	HAWK_EBADVAR,   /**< '${0}' not a valid variable name */
	HAWK_EVARMS,    /**< variable name missing */
	HAWK_EUNDEF,    /**< undefined identifier '${0}' */
	HAWK_ELVALUE,   /**< l-value required */
	HAWK_EGBLTM,    /**< too many global variables */
	HAWK_ELCLTM,    /**< too many local variables */
	HAWK_EPARTM,    /**< too many parameters */
	HAWK_ESEGTM,    /**< too many segments */
	HAWK_ESEGTL,    /**< segment '${0}' too long */
	HAWK_EBADARG,   /**< bad argument */
	HAWK_ENOARG,    /**< no argument */
	HAWK_EBREAK,    /**< 'break' outside a loop */
	HAWK_ECONTINUE, /**< 'continue' outside a loop */
	HAWK_ENEXTBEG,  /**< 'next' illegal in BEGIN block */
	HAWK_ENEXTEND,  /**< 'next' illegal in END block */
	HAWK_ENEXTFBEG, /**< 'nextfile' illegal in BEGIN block */
	HAWK_ENEXTFEND, /**< 'nextfile' illegal in END block */
	HAWK_EPREPST,   /**< both prefix and postfix incr/decr operator present */
	HAWK_EINCDECOPR,/**< illegal operand for incr/decr operator */
	HAWK_EINCLSTR,  /**< 'include' not followed by a string */
	HAWK_EINCLTD,   /**< include level too deep */
	HAWK_EXKWNR,    /**< @word '${0}' not recognized */
	HAWK_EXKWEM,    /**< @ not followed by a valid word  */

	/* run time error */
	HAWK_ESTACK,        /**< stack error */
	HAWK_EDIVBY0,       /**< divide by zero */
	HAWK_EOPERAND,      /**< invalid operand */
	HAWK_EPOSIDX,       /**< wrong position index */
	HAWK_EARGTF,        /**< too few arguments */
	HAWK_EARGTM,        /**< too many arguments */
	HAWK_EFUNNF,        /**< function '${0}' not found */
	HAWK_ENOTFUN,       /**< non-function value in '%{0}' */
	HAWK_ENOTDEL,       /**< '${0}' not deletable */
	HAWK_ENOTMAP,       /**< value not a map */
	HAWK_ENOTMAPIN,     /**< right-hand side of 'in' not a map */
	HAWK_ENOTMAPNILIN,  /**< right-hand side of 'in' not a map nor nil */
	HAWK_ENOTREF,       /**< value not referenceable */
	HAWK_EMAPRET,       /**< map cannot be returned */
	HAWK_EMAPTOPOS,     /**< map cannot be assigned to a positional */
	HAWK_EMAPTOIDX,     /**< map cannot be assigned to an indexed variable */
	HAWK_EMAPTONVAR,    /**< map cannot be assigned to an variable '${0}' */
	HAWK_EMAPTOSCALAR,  /**< cannot change a map to a scalar value */
	HAWK_ESCALARTOMAP,  /**< cannot change a scalar to a map */
	HAWK_ENMAPTOMAP,    /**< cannot change a map '${0}' to another map */
	HAWK_ENMAPTOSCALAR, /**< cannot change a map '${0}' to a scalar */
	HAWK_ENSCALARTOMAP, /**< cannot change a scalar '${0}' to a map */
	HAWK_EVALTOSTR,     /**< invalid value to convert to a string */
	HAWK_EVALTONUM,     /**< invalid value to convert to a number */
	HAWK_EVALTOCHR,     /**< invalid value to convert to a character */
	HAWK_EHASHVAL,      /**< invalid value to hash */
	HAWK_ERNEXTBEG,     /**< 'next' called from BEGIN block */
	HAWK_ERNEXTEND,     /**< 'next' called from END block */
	HAWK_ERNEXTFBEG,    /**< 'nextfile' called from BEGIN block */
	HAWK_ERNEXTFEND,    /**< 'nextfile' called from END block */
	HAWK_EFNCIMPL,      /**< intrinsic function handler for '${0}' failed */
	HAWK_EIOUSER,       /**< wrong user io handler implementation */
	HAWK_EIOIMPL,       /**< I/O callback returned an error */
	HAWK_EIONMNF,       /**< no such I/O name found */
	HAWK_EIONMEM,       /**< I/O name empty */
	HAWK_EIONMNL,       /**< I/O name '${0}' containing '\\0' */
	HAWK_EFMTARG,       /**< not sufficient arguments to formatting sequence */
	HAWK_EFMTCNV,       /**< recursion detected in format conversion */
	HAWK_ECONVFMTCHR,   /**< invalid character in CONVFMT */
	HAWK_EOFMTCHR,      /**< invalid character in OFMT */

	/* regular expression error */
	HAWK_EREXBL,        /**< failed to build regular expression */
	HAWK_EREXMA,        /**< failed to match regular expression */
	HAWK_EREXRECUR,     /**< recursion too deep */
	HAWK_EREXRPAREN,    /**< a right parenthesis is expected */
	HAWK_EREXRBRACK,    /**< a right bracket is expected */
	HAWK_EREXRBRACE,    /**< a right brace is expected */
	HAWK_EREXCOLON,     /**< a colon is expected */
	HAWK_EREXCRANGE,    /**< invalid character range */
	HAWK_EREXCCLASS,    /**< invalid character class */
	HAWK_EREXBOUND,     /**< invalid occurrence bound */
	HAWK_EREXSPCAWP,    /**< special character at wrong position */
	HAWK_EREXPREEND,    /**< premature end of regular expression */

	/* the number of error numbers, internal use only */
	HAWK_NUMERRNUM 
};
typedef enum hawk_errnum_t hawk_errnum_t;

struct hawk_gem_t
{
	hawk_mmgr_t*  mmgr;
	hawk_cmgr_t*  cmgr;
	hawk_errnum_t errnum;
	hawk_ooch_t   errmsg[HAWK_ERRMSG_CAPA];
};

/* =========================================================================
 * MACROS THAT CHANGES THE BEHAVIORS OF THE C COMPILER/LINKER
 * =========================================================================*/

#if defined(EMSCRIPTEN)
#	define HAWK_IMPORT
#	define HAWK_EXPORT EMSCRIPTEN_KEEPALIVE
#	define HAWK_PRIVATE
#elif defined(__BORLANDC__) && (__BORLANDC__ < 0x500)
#	define HAWK_IMPORT
#	define HAWK_EXPORT
#	define HAWK_PRIVATE
#elif defined(_WIN32) || (defined(__WATCOMC__) && (__WATCOMC__ >= 1000) && !defined(__WINDOWS_386__))
#	define HAWK_IMPORT __declspec(dllimport)
#	define HAWK_EXPORT __declspec(dllexport)
#	define HAWK_PRIVATE 
#elif defined(__GNUC__) && ((__GNUC__>= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3))
#	define HAWK_IMPORT __attribute__((visibility("default")))
#	define HAWK_EXPORT __attribute__((visibility("default")))
#	define HAWK_PRIVATE __attribute__((visibility("hidden")))
/*#	define HAWK_PRIVATE __attribute__((visibility("internal")))*/
#else
#	define HAWK_IMPORT
#	define HAWK_EXPORT
#	define HAWK_PRIVATE
#endif

#if defined(__cplusplus) || (defined(__STDC_VERSION__) && (__STDC_VERSION__>=199901L))
	/* C++/C99 has inline */
#	define HAWK_INLINE inline
#	define HAWK_HAVE_INLINE
#elif defined(__GNUC__) && defined(__GNUC_GNU_INLINE__)
	/* gcc disables inline when -std=c89 or -ansi is used. 
	 * so use __inline__ supported by gcc regardless of the options */
#	define HAWK_INLINE /*extern*/ __inline__
#	define HAWK_HAVE_INLINE
#else
#	define HAWK_INLINE 
#	undef HAWK_HAVE_INLINE
#endif

#if defined(__GNUC__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4))
#	define HAWK_UNUSED __attribute__((__unused__))
#else
#	define HAWK_UNUSED
#endif

/**
 * The HAWK_TYPE_IS_SIGNED() macro determines if a type is signed.
 * \code
 * printf ("%d\n", (int)HAWK_TYPE_IS_SIGNED(int));
 * printf ("%d\n", (int)HAWK_TYPE_IS_SIGNED(unsigned int));
 * \endcode
 */
#define HAWK_TYPE_IS_SIGNED(type) (((type)0) > ((type)-1))

/**
 * The HAWK_TYPE_IS_SIGNED() macro determines if a type is unsigned.
 * \code
 * printf ("%d\n", HAWK_TYPE_IS_UNSIGNED(int));
 * printf ("%d\n", HAWK_TYPE_IS_UNSIGNED(unsigned int));
 * \endcode
 */
#define HAWK_TYPE_IS_UNSIGNED(type) (((type)0) < ((type)-1))

#define HAWK_TYPE_SIGNED_MAX(type) \
	((type)~((type)1 << ((type)HAWK_BITSOF(type) - 1)))
#define HAWK_TYPE_UNSIGNED_MAX(type) ((type)(~(type)0))

#define HAWK_TYPE_SIGNED_MIN(type) \
	((type)((type)1 << ((type)HAWK_BITSOF(type) - 1)))
#define HAWK_TYPE_UNSIGNED_MIN(type) ((type)0)

#define HAWK_TYPE_MAX(type) \
	((HAWK_TYPE_IS_SIGNED(type)? HAWK_TYPE_SIGNED_MAX(type): HAWK_TYPE_UNSIGNED_MAX(type)))
#define HAWK_TYPE_MIN(type) \
	((HAWK_TYPE_IS_SIGNED(type)? HAWK_TYPE_SIGNED_MIN(type): HAWK_TYPE_UNSIGNED_MIN(type)))

/* round up a positive integer x to the nearst multiple of y */
#define HAWK_ALIGN(x,y) ((((x) + (y) - 1) / (y)) * (y))

/* round up a positive integer x to the nearst multiple of y where
 * y must be a multiple of a power of 2*/
#define HAWK_ALIGN_POW2(x,y) ((((x) + (y) - 1)) & ~((y) - 1))

#define HAWK_IS_UNALIGNED_POW2(x,y) ((x) & ((y) - 1))
#define HAWK_IS_ALIGNED_POW2(x,y) (!HAWK_IS_UNALIGNED_POW2(x,y))

/* =========================================================================
 * COMPILER FEATURE TEST MACROS
 * =========================================================================*/
#if !defined(__has_builtin) && defined(_INTELC32_)
	/* intel c code builder 1.0 ended up with an error without this override */
	#define __has_builtin(x) 0
#endif

/*
#if !defined(__is_identifier)
	#define __is_identifier(x) 0
#endif

#if !defined(__has_attribute)
	#define __has_attribute(x) 0
#endif
*/

#if defined(__has_builtin) 
	#if __has_builtin(__builtin_ctz)
		#define HAWK_HAVE_BUILTIN_CTZ
	#endif
	#if __has_builtin(__builtin_ctzl)
		#define HAWK_HAVE_BUILTIN_CTZL
	#endif
	#if __has_builtin(__builtin_ctzll)
		#define HAWK_HAVE_BUILTIN_CTZLL
	#endif

	#if __has_builtin(__builtin_clz)
		#define HAWK_HAVE_BUILTIN_CLZ
	#endif
	#if __has_builtin(__builtin_clzl)
		#define HAWK_HAVE_BUILTIN_CLZL
	#endif
	#if __has_builtin(__builtin_clzll)
		#define HAWK_HAVE_BUILTIN_CLZLL
	#endif

	#if __has_builtin(__builtin_uadd_overflow)
		#define HAWK_HAVE_BUILTIN_UADD_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_uaddl_overflow)
		#define HAWK_HAVE_BUILTIN_UADDL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_uaddll_overflow)
		#define HAWK_HAVE_BUILTIN_UADDLL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_umul_overflow)
		#define HAWK_HAVE_BUILTIN_UMUL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_umull_overflow)
		#define HAWK_HAVE_BUILTIN_UMULL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_umulll_overflow)
		#define HAWK_HAVE_BUILTIN_UMULLL_OVERFLOW 
	#endif

	#if __has_builtin(__builtin_sadd_overflow)
		#define HAWK_HAVE_BUILTIN_SADD_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_saddl_overflow)
		#define HAWK_HAVE_BUILTIN_SADDL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_saddll_overflow)
		#define HAWK_HAVE_BUILTIN_SADDLL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_smul_overflow)
		#define HAWK_HAVE_BUILTIN_SMUL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_smull_overflow)
		#define HAWK_HAVE_BUILTIN_SMULL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_smulll_overflow)
		#define HAWK_HAVE_BUILTIN_SMULLL_OVERFLOW 
	#endif

	#if __has_builtin(__builtin_expect)
		#define HAWK_HAVE_BUILTIN_EXPECT
	#endif


	#if __has_builtin(__sync_lock_test_and_set)
		#define HAWK_HAVE_SYNC_LOCK_TEST_AND_SET
	#endif
	#if __has_builtin(__sync_lock_release)
		#define HAWK_HAVE_SYNC_LOCK_RELEASE
	#endif

	#if __has_builtin(__sync_synchronize)
		#define HAWK_HAVE_SYNC_SYNCHRONIZE
	#endif
	#if __has_builtin(__sync_bool_compare_and_swap)
		#define HAWK_HAVE_SYNC_BOOL_COMPARE_AND_SWAP
	#endif
	#if __has_builtin(__sync_val_compare_and_swap)
		#define HAWK_HAVE_SYNC_VAL_COMPARE_AND_SWAP
	#endif


	#if __has_builtin(__builtin_bswap16)
		#define HAWK_HAVE_BUILTIN_BSWAP16
	#endif
	#if __has_builtin(__builtin_bswap32)
		#define HAWK_HAVE_BUILTIN_BSWAP32
	#endif
	#if __has_builtin(__builtin_bswap64)
		#define HAWK_HAVE_BUILTIN_BSWAP64
	#endif
	#if __has_builtin(__builtin_bswap128)
		#define HAWK_HAVE_BUILTIN_BSWAP128
	#endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)

	#if (__GNUC__ >= 4) 
		#define HAWK_HAVE_SYNC_LOCK_TEST_AND_SET
		#define HAWK_HAVE_SYNC_LOCK_RELEASE

		#define HAWK_HAVE_SYNC_SYNCHRONIZE
		#define HAWK_HAVE_SYNC_BOOL_COMPARE_AND_SWAP
		#define HAWK_HAVE_SYNC_VAL_COMPARE_AND_SWAP
	#endif

	#if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
		#define HAWK_HAVE_BUILTIN_CTZ
		#define HAWK_HAVE_BUILTIN_CTZL
		#define HAWK_HAVE_BUILTIN_CTZLL
		#define HAWK_HAVE_BUILTIN_CLZ
		#define HAWK_HAVE_BUILTIN_CLZL
		#define HAWK_HAVE_BUILTIN_CLZLL
		#define HAWK_HAVE_BUILTIN_EXPECT
	#endif

	#if (__GNUC__ >= 5)
		#define HAWK_HAVE_BUILTIN_UADD_OVERFLOW
		#define HAWK_HAVE_BUILTIN_UADDL_OVERFLOW
		#define HAWK_HAVE_BUILTIN_UADDLL_OVERFLOW
		#define HAWK_HAVE_BUILTIN_UMUL_OVERFLOW
		#define HAWK_HAVE_BUILTIN_UMULL_OVERFLOW
		#define HAWK_HAVE_BUILTIN_UMULLL_OVERFLOW

		#define HAWK_HAVE_BUILTIN_SADD_OVERFLOW
		#define HAWK_HAVE_BUILTIN_SADDL_OVERFLOW
		#define HAWK_HAVE_BUILTIN_SADDLL_OVERFLOW
		#define HAWK_HAVE_BUILTIN_SMUL_OVERFLOW
		#define HAWK_HAVE_BUILTIN_SMULL_OVERFLOW
		#define HAWK_HAVE_BUILTIN_SMULLL_OVERFLOW
	#endif

	#if (__GNUC__ >= 5) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
		/* 4.8.0 or later */
		#define HAWK_HAVE_BUILTIN_BSWAP16
	#endif
	#if (__GNUC__ >= 5) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
		/* 4.3.0 or later */
		#define HAWK_HAVE_BUILTIN_BSWAP32
		#define HAWK_HAVE_BUILTIN_BSWAP64
		/*#define HAWK_HAVE_BUILTIN_BSWAP128*/
	#endif
#endif

#if defined(HAWK_HAVE_BUILTIN_EXPECT)
#	define HAWK_LIKELY(x) (__builtin_expect(!!(x),1))
#	define HAWK_UNLIKELY(x) (__builtin_expect(!!(x),0))
#else
#	define HAWK_LIKELY(x) (x)
#	define HAWK_UNLIKELY(x) (x)
#endif


/* =========================================================================
 * STATIC ASSERTION
 * =========================================================================*/
#define HAWK_STATIC_JOIN_INNER(x, y) x ## y
#define HAWK_STATIC_JOIN(x, y) HAWK_STATIC_JOIN_INNER(x, y)

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#	define HAWK_STATIC_ASSERT(expr)  _Static_assert (expr, "invalid assertion")
#elif defined(__cplusplus) && (__cplusplus >= 201103L)
#	define HAWK_STATIC_ASSERT(expr) static_assert (expr, "invalid assertion")
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#	define HAWK_STATIC_ASSERT(expr) typedef char HAWK_STATIC_JOIN(HAWK_STATIC_ASSERT_T_, __LINE__)[(expr)? 1: -1] HAWK_UNUSED
#else
#	define HAWK_STATIC_ASSERT(expr) do { typedef char HAWK_STATIC_JOIN(HAWK_STATIC_ASSERT_T_, __LINE__)[(expr)? 1: -1] HAWK_UNUSED; } while(0)
#endif

#define HAWK_STATIC_ASSERT_EXPR(expr) ((void)HAWK_SIZEOF(char[(expr)? 1: -1]))


/* =========================================================================
 * CHARACTER/STRING LITERALS
 * =========================================================================*/
#define HAWK_MQ_I(val) #val
#define HAWK_MQ(val)   HAWK_MQ_I(val)
/**
 * The #HAWK_BT macro maps a multi-byte literal string literal as it is.
 */
#define HAWK_BT(txt)   (txt)

#if defined(HAWK_UCH_IS_CHAR16_T)
#       define HAWK_WQ_I(val)  (u ## #val)
#       define HAWK_WQ(val)    HAWK_WQ_I(val)
#else
#       define HAWK_WQ_I(val)  (L ## #val)
#       define HAWK_WQ(val)    HAWK_WQ_I(val)
#endif

/**
 * The #HAWK_UT macro maps a multi-byte literal string to a wide character
 * string by prefixing it with \b L.
 */
#if (HAWK_SIZEOF_UCH_T == HAWK_SIZEOF_BCH_T)
#       define HAWK_UT(txt)    (txt)
#elif defined(HAWK_UCH_IS_CHAR16_T)
#       define HAWK_UT(txt)    (u ## txt)
#else
#       define HAWK_UT(txt)    (L ## txt)
#endif

/** \def HAWK_T
 * The #HAWK_T macro maps to #HAWK_BT if #HAWK_OOCH_IS_BCH is defined, and to
 * #HAWK_UT if #HAWK_OOCH_IS_UCH is defined.
 */
#if defined(HAWK_OOCH_IS_BCH)
#       define HAWK_Q(val) HAWK_MQ(val)
#       define HAWK_T(txt) HAWK_BT(txt)
#else
#       define HAWK_Q(val) HAWK_WQ(val)
#       define HAWK_T(txt) HAWK_UT(txt)
#endif


/**
 * The #hawk_foff_t type defines an integer that can represent a file offset.
 * Depending on your system, it's defined to one of #hawk_int64_t, #hawk_int32_t,
 * and #hawk_int16_t.
 */
#if defined(HAWK_HAVE_INT64_T) && (HAWK_SIZEOF_OFF64_T==8)
	typedef hawk_int64_t hawk_foff_t;
#	define HAWK_SIZEOF_FOFF_T HAWK_SIZEOF_INT64_T
#elif defined(HAWK_HAVE_INT64_T) && (HAWK_SIZEOF_OFF_T==8)
	typedef hawk_int64_t hawk_foff_t;
#	define HAWK_SIZEOF_FOFF_T HAWK_SIZEOF_INT64_T
#elif defined(HAWK_HAVE_INT32_T) && (HAWK_SIZEOF_OFF_T==4)
	typedef hawk_int32_t hawk_foff_t;
#	define HAWK_SIZEOF_FOFF_T HAWK_SIZEOF_INT32_T
#elif defined(HAWK_HAVE_INT16_T) && (HAWK_SIZEOF_OFF_T==2)
	typedef hawk_int16_t hawk_foff_t;
#	define HAWK_SIZEOF_FOFF_T HAWK_SIZEOF_INT16_T
#elif defined(HAWK_HAVE_INT8_T) && (HAWK_SIZEOF_OFF_T==1)
	typedef hawk_int8_t hawk_foff_t;
#	define HAWK_SIZEOF_FOFF_T HAWK_SIZEOF_INT16_T
#else
	typedef hawk_int32_t hawk_foff_t; /* this line is for doxygen */
#       error Unsupported platform
#endif


/**** TODO ****/
#define HAWK_ASSERT(hawk, x)

#endif
