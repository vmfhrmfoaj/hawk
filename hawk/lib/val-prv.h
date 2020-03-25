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

#ifndef _HAWK_VAL_PRV_H_
#define _HAWK_VAL_PRV_H_

#define HAWK_VAL_CHUNK_SIZE 100

typedef struct hawk_val_chunk_t hawk_val_chunk_t;
typedef struct hawk_val_ichunk_t hawk_val_ichunk_t;
typedef struct hawk_val_rchunk_t hawk_val_rchunk_t;

struct hawk_val_chunk_t
{
        hawk_val_chunk_t* next;
};

struct hawk_val_ichunk_t
{
	hawk_val_chunk_t* next;
	/* make sure that it has the same fields as 
	   hawk_val_chunk_t up to this point */

	hawk_val_int_t slot[HAWK_VAL_CHUNK_SIZE];
};

struct hawk_val_rchunk_t
{
	hawk_val_chunk_t* next;
	/* make sure that it has the same fields as 
	   hawk_val_chunk_t up to this point */

	hawk_val_flt_t slot[HAWK_VAL_CHUNK_SIZE];
};


/* 
 * if shared objects link a static library, statically defined objects
 * in the static library will be instatiated in the multiple shared objects.
 *
 * so equality check with a value pointer doesn't work
 * if the code crosses the library boundaries. instead, i decided to
 * add a field to indicate if a value is static.
 * 

#define IS_STATICVAL(val) ((val) == HAWK_NULL || (val) == hawk_val_nil || (val) == hawk_val_zls || (val) == hawk_val_zlm)
*/
#define IS_STATICVAL(val) ((val)->v_static)


/* hawk_val_t pointer encoding assumes the pointer is an even number.
 * i shift an integer within a certain range and set bit 0 to 1 to
 * encode it in a pointer. (vtr = value pointer).
 *
 * is this a safe assumption? do i have to use memalign or write my own
 * aligned malloc()? */
#define HAWK_VTR_NUM_TYPE_BITS        1
#define HAWK_VTR_MASK_TYPE_BITS       1 

#define HAWK_VTR_TYPE_BITS_POINTER    0
#define HAWK_VTR_TYPE_BITS_QUICKINT   1
#define HAWK_VTR_SIGN_BIT ((hawk_uintptr_t)1 << (HAWK_SIZEOF_UINTPTR_T * 8 - 1))

/* shrink the bit range by 1 more bit to ease signbit handling. 
 * i want abs(max) == abs(min).
 * i don't want abs(max) + 1 == abs(min). e.g min: -32768, max: 32767
 */
#define HAWK_QUICKINT_MAX ((hawk_int_t)((~(hawk_uintptr_t)0) >> (HAWK_VTR_NUM_TYPE_BITS + 1)))
#define HAWK_QUICKINT_MIN (-HAWK_QUICKINT_MAX)
#define HAWK_IN_QUICKINT_RANGE(i) ((i) >= HAWK_QUICKINT_MIN && (i) <= HAWK_QUICKINT_MAX)

#define HAWK_VTR_TYPE_BITS(p) (((hawk_uintptr_t)(p)) & HAWK_VTR_MASK_TYPE_BITS)
#define HAWK_VTR_IS_POINTER(p) (HAWK_VTR_TYPE_BITS(p) == HAWK_VTR_TYPE_BITS_POINTER)
#define HAWK_VTR_IS_QUICKINT(p) (HAWK_VTR_TYPE_BITS(p) == HAWK_VTR_TYPE_BITS_QUICKINT)

#define HAWK_QUICKINT_TO_VTR_POSITIVE(i) \
	(((hawk_uintptr_t)(i) << HAWK_VTR_NUM_TYPE_BITS) | HAWK_VTR_TYPE_BITS_QUICKINT)

#define HAWK_QUICKINT_TO_VTR_NEGATIVE(i) \
	((((hawk_uintptr_t)-(i)) << HAWK_VTR_NUM_TYPE_BITS) | HAWK_VTR_TYPE_BITS_QUICKINT | HAWK_VTR_SIGN_BIT)

#define HAWK_QUICKINT_TO_VTR(i) \
	((hawk_val_t*)(((i) < 0)? HAWK_QUICKINT_TO_VTR_NEGATIVE(i): HAWK_QUICKINT_TO_VTR_POSITIVE(i)))

#define HAWK_VTR_ZERO ((hawk_val_t*)HAWK_QUICKINT_TO_VTR_POSITIVE(0))
#define HAWK_VTR_ONE  ((hawk_val_t*)HAWK_QUICKINT_TO_VTR_POSITIVE(1))
#define HAWK_VTR_NEGONE ((hawk_val_t*)HAWK_QUICKINT_TO_VTR_NEGATIVE(-1))

/* sizeof(hawk_intptr_t) may not be the same as sizeof(hawk_int_t).
 * so step-by-step type conversions are needed.
 * e.g) pointer to uintptr_t, uintptr_t to intptr_t, intptr_t to awk_int_t */
#define HAWK_VTR_TO_QUICKINT_POSITIVE(p) \
	((hawk_intptr_t)((hawk_uintptr_t)(p) >> HAWK_VTR_NUM_TYPE_BITS))
#define HAWK_VTR_TO_QUICKINT_NEGATIVE(p) \
	(-(hawk_intptr_t)(((hawk_uintptr_t)(p) & ~HAWK_VTR_SIGN_BIT) >> HAWK_VTR_NUM_TYPE_BITS))
#define HAWK_VTR_TO_QUICKINT(p) \
	(((hawk_uintptr_t)(p) & HAWK_VTR_SIGN_BIT)? HAWK_VTR_TO_QUICKINT_NEGATIVE(p): HAWK_VTR_TO_QUICKINT_POSITIVE(p))


#define HAWK_RTX_GETVALTYPE(rtx,p) (HAWK_VTR_IS_QUICKINT(p)? HAWK_VAL_INT: (p)->v_type)
#define HAWK_RTX_GETINTFROMVAL(rtx,p) ((HAWK_VTR_IS_QUICKINT(p)? (hawk_int_t)HAWK_VTR_TO_QUICKINT(p): ((hawk_val_int_t*)(p))->i_val))


#define HAWK_VAL_ZERO HAWK_VTR_ZERO
#define HAWK_VAL_ONE  HAWK_VTR_ONE
#define HAWK_VAL_NEGONE HAWK_VTR_NEGONE


#define HAWK_RTX_FREEVAL_CACHE       (1 << 0)
#define HAWK_RTX_FREEVAL_GC_PRESERVE (1 << 1)

#if defined(__cplusplus)
extern "C" {
#endif

/* represents a nil value */
extern hawk_val_t* hawk_val_nil;

/* represents an empty string  */
extern hawk_val_t* hawk_val_zls;


void hawk_rtx_freeval (
        hawk_rtx_t* rtx,
        hawk_val_t* val,
        int         flags
);

void hawk_rtx_freevalchunk (
	hawk_rtx_t*       rtx,
	hawk_val_chunk_t* chunk
);

#if defined(__cplusplus)
}
#endif

#endif
