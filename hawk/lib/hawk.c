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

static void free_fun (hawk_htb_t* map, void* vptr, hawk_oow_t vlen)
{
	hawk_t* awk = *(hawk_t**)(map + 1);
	hawk_fun_t* f = (hawk_fun_t*)vptr;

	/* f->name doesn't have to be freed */
	/*hawk_freemem (awk, f->name);*/

	if (f->argspec) hawk_freemem (awk, f->argspec);
	hawk_clrpt (awk, f->body);
	hawk_freemem (awk, f);
}

static void free_fnc (hawk_htb_t* map, void* vptr, hawk_oow_t vlen)
{
	hawk_t* awk = *(hawk_t**)(map + 1);
	hawk_fnc_t* f = (hawk_fnc_t*)vptr;
	hawk_freemem (awk, f);
}

static int init_token (hawk_t* hawk, hawk_tok_t* tok)
{
	tok->name = hawk_ooecs_open(hawk_getgem(hawk), 0, 128);
	if (!tok->name) return -1;

	tok->type = 0;
	tok->loc.file = HAWK_NULL;
	tok->loc.line = 0;
	tok->loc.colm = 0;

	return 0;
}

static void fini_token (hawk_tok_t* tok)
{
	if (tok->name)
	{
		hawk_ooecs_close (tok->name);
		tok->name = HAWK_NULL;
	}
}

static void clear_token (hawk_tok_t* tok)
{
	if (tok->name) hawk_ooecs_clear (tok->name);
	tok->type = 0;
	tok->loc.file = HAWK_NULL;
	tok->loc.line = 0;
	tok->loc.colm = 0;
}

hawk_t* hawk_open (hawk_mmgr_t* mmgr, hawk_oow_t xtnsize, hawk_cmgr_t* cmgr, const hawk_prm_t* prm, hawk_errnum_t* errnum)
{
	hawk_t* awk;

	awk = (hawk_t*)HAWK_MMGR_ALLOC(mmgr, HAWK_SIZEOF(hawk_t) + xtnsize);
	if (awk)
	{
		int xret;

		xret = hawk_init(awk, mmgr, cmgr, prm);
		if (xret <= -1)
		{
			if (errnum) *errnum = hawk_geterrnum(awk);
			HAWK_MMGR_FREE (mmgr, awk);
			awk = HAWK_NULL;
		}
		else HAWK_MEMSET (awk + 1, 0, xtnsize);
	}
	else if (errnum) *errnum = HAWK_ENOMEM;

	return awk;
}

void hawk_close (hawk_t* hawk)
{
	hawk_fini (hawk);
	HAWK_MMGR_FREE (hawk_getmmgr(hawk), hawk);
}

int hawk_init (hawk_t* awk, hawk_mmgr_t* mmgr, hawk_cmgr_t* cmgr, const hawk_prm_t* prm)
{
	static hawk_htb_style_t treefuncbs =
	{
		{
			HAWK_HTB_COPIER_INLINE,
			HAWK_HTB_COPIER_DEFAULT 
		},
		{
			HAWK_HTB_FREEER_DEFAULT,
			free_fun
		},
		HAWK_HTB_COMPER_DEFAULT,
		HAWK_HTB_KEEPER_DEFAULT,
		HAWK_HTB_SIZER_DEFAULT,
		HAWK_HTB_HASHER_DEFAULT
	};

	static hawk_htb_style_t fncusercbs =
	{
		{
			HAWK_HTB_COPIER_INLINE,
			HAWK_HTB_COPIER_DEFAULT 
		},
		{
			HAWK_HTB_FREEER_DEFAULT,
			free_fnc
		},
		HAWK_HTB_COMPER_DEFAULT,
		HAWK_HTB_KEEPER_DEFAULT,
		HAWK_HTB_SIZER_DEFAULT,
		HAWK_HTB_HASHER_DEFAULT
	};

	/* zero out the object */
	HAWK_MEMSET (awk, 0, HAWK_SIZEOF(*awk));

	/* remember the memory manager */
	awk->_instsize = HAWK_SIZEOF(*awk);
	awk->_gem.mmgr = mmgr;
	awk->_gem.cmgr = cmgr;

	/* initialize error handling fields */
	awk->_gem.errnum = HAWK_ENOERR;
	awk->_gem.errmsg[0] = '\0';
	awk->_gem.errloc.line = 0;
	awk->_gem.errloc.colm = 0;
	awk->_gem.errloc.file = HAWK_NULL;
	awk->errstr = hawk_dfl_errstr;
	awk->haltall = 0;

	/* progagate the primitive functions */
	HAWK_ASSERT (prm           != HAWK_NULL);
	HAWK_ASSERT (prm->math.pow != HAWK_NULL);
	HAWK_ASSERT (prm->math.mod != HAWK_NULL);
	if (prm           == HAWK_NULL || 
	    prm->math.pow == HAWK_NULL ||
	    prm->math.mod == HAWK_NULL)
	{
		hawk_seterrnum (awk, HAWK_NULL, HAWK_EINVAL);
		goto oops;
	}
	awk->prm = *prm;

	if (init_token(awk, &awk->ptok) <= -1 ||
	    init_token(awk, &awk->tok) <= -1 ||
	    init_token(awk, &awk->ntok) <= -1) goto oops;

	awk->opt.trait = HAWK_MODERN;
#if defined(__WIN32__) || defined(_OS2) || defined(__DOS__)
	awk->opt.trait |= HAWK_CRLF;
#endif
	awk->opt.rtx_stack_limit = HAWK_DFL_RTX_STACK_LIMIT;
	awk->opt.log_mask = HAWK_LOG_ALL_LEVELS  | HAWK_LOG_ALL_TYPES;
	awk->opt.log_maxcapa = HAWK_DFL_LOG_MAXCAPA;

	awk->log.capa = HAWK_ALIGN_POW2(1, HAWK_LOG_CAPA_ALIGN);
	awk->log.ptr = hawk_allocmem(awk, (awk->log.capa + 1) * HAWK_SIZEOF(*awk->log.ptr));
	if (!awk->log.ptr) goto oops;

	awk->tree.ngbls = 0;
	awk->tree.ngbls_base = 0;
	awk->tree.begin = HAWK_NULL;
	awk->tree.begin_tail = HAWK_NULL;
	awk->tree.end = HAWK_NULL;
	awk->tree.end_tail = HAWK_NULL;
	awk->tree.chain = HAWK_NULL;
	awk->tree.chain_tail = HAWK_NULL;
	awk->tree.chain_size = 0;

	/* TODO: initial map size?? */
	awk->tree.funs = hawk_htb_open(hawk_getgem(awk), HAWK_SIZEOF(awk), 512, 70, HAWK_SIZEOF(hawk_ooch_t), 1);
	awk->parse.funs = hawk_htb_open(hawk_getgem(awk), HAWK_SIZEOF(awk), 256, 70, HAWK_SIZEOF(hawk_ooch_t), 1);
	awk->parse.named = hawk_htb_open(hawk_getgem(awk), HAWK_SIZEOF(awk), 256, 70, HAWK_SIZEOF(hawk_ooch_t), 1);

	awk->parse.gbls = hawk_arr_open(hawk_getgem(awk), HAWK_SIZEOF(awk), 128);
	awk->parse.lcls = hawk_arr_open(hawk_getgem(awk), HAWK_SIZEOF(awk), 64);
	awk->parse.params = hawk_arr_open(hawk_getgem(awk), HAWK_SIZEOF(awk), 32);

	awk->fnc.sys = HAWK_NULL;
	awk->fnc.user = hawk_htb_open(hawk_getgem(awk), HAWK_SIZEOF(awk), 512, 70, HAWK_SIZEOF(hawk_ooch_t), 1);
	awk->modtab = hawk_rbt_open(hawk_getgem(awk), 0, HAWK_SIZEOF(hawk_ooch_t), 1);

	if (awk->tree.funs == HAWK_NULL ||
	    awk->parse.funs == HAWK_NULL ||
	    awk->parse.named == HAWK_NULL ||
	    awk->parse.gbls == HAWK_NULL ||
	    awk->parse.lcls == HAWK_NULL ||
	    awk->parse.params == HAWK_NULL ||
	    awk->fnc.user == HAWK_NULL ||
	    awk->modtab == HAWK_NULL) 
	{
		hawk_seterrnum (awk, HAWK_NULL, HAWK_ENOMEM);
		goto oops;
	}

	*(hawk_t**)(awk->tree.funs + 1) = awk;
	hawk_htb_setstyle (awk->tree.funs, &treefuncbs);

	*(hawk_t**)(awk->parse.funs + 1) = awk;
	hawk_htb_setstyle (awk->parse.funs, hawk_get_htb_style(HAWK_HTB_STYLE_INLINE_KEY_COPIER));

	*(hawk_t**)(awk->parse.named + 1) = awk;
	hawk_htb_setstyle (awk->parse.named, hawk_get_htb_style(HAWK_HTB_STYLE_INLINE_KEY_COPIER));

	*(hawk_t**)(awk->parse.gbls + 1) = awk;
	hawk_arr_setscale (awk->parse.gbls, HAWK_SIZEOF(hawk_ooch_t));
	hawk_arr_setcopier (awk->parse.gbls, HAWK_ARR_COPIER_INLINE);

	*(hawk_t**)(awk->parse.lcls + 1) = awk;
	hawk_arr_setscale (awk->parse.lcls, HAWK_SIZEOF(hawk_ooch_t));
	hawk_arr_setcopier (awk->parse.lcls, HAWK_ARR_COPIER_INLINE);

	*(hawk_t**)(awk->parse.params + 1) = awk;
	hawk_arr_setscale (awk->parse.params, HAWK_SIZEOF(hawk_ooch_t));
	hawk_arr_setcopier (awk->parse.params, HAWK_ARR_COPIER_INLINE);

	*(hawk_t**)(awk->fnc.user + 1) = awk;
	hawk_htb_setstyle (awk->fnc.user, &fncusercbs);

	hawk_rbt_setstyle (awk->modtab, hawk_get_rbt_style(HAWK_RBT_STYLE_INLINE_COPIERS));

	if (hawk_initgbls(awk) <= -1) goto oops;
	return 0;

oops:
	if (awk->modtab) hawk_rbt_close (awk->modtab);
	if (awk->fnc.user) hawk_htb_close (awk->fnc.user);
	if (awk->parse.params) hawk_arr_close (awk->parse.params);
	if (awk->parse.lcls) hawk_arr_close (awk->parse.lcls);
	if (awk->parse.gbls) hawk_arr_close (awk->parse.gbls);
	if (awk->parse.named) hawk_htb_close (awk->parse.named);
	if (awk->parse.funs) hawk_htb_close (awk->parse.funs);
	if (awk->tree.funs) hawk_htb_close (awk->tree.funs);
	fini_token (&awk->ntok);
	fini_token (&awk->tok);
	fini_token (&awk->ptok);
	if (awk->log.ptr) hawk_freemem (awk, awk->log.ptr);
	awk->log.capa = 0;

	return -1;
}

void hawk_fini (hawk_t* awk)
{
	hawk_ecb_t* ecb;
	int i;

	hawk_clear (awk);
	/*hawk_clrfnc (awk);*/

	if (awk->log.len >  0)
        {
		int shuterr = awk->shuterr;
		awk->shuterr = 1;
		awk->prm.logwrite (awk, awk->log.last_mask, awk->log.ptr, awk->log.len);
		awk->shuterr = shuterr;
	}

	for (ecb = awk->ecb; ecb; ecb = ecb->next)
		if (ecb->close) ecb->close (awk);

	hawk_rbt_close (awk->modtab);
	hawk_htb_close (awk->fnc.user);

	hawk_arr_close (awk->parse.params);
	hawk_arr_close (awk->parse.lcls);
	hawk_arr_close (awk->parse.gbls);
	hawk_htb_close (awk->parse.named);
	hawk_htb_close (awk->parse.funs);

	hawk_htb_close (awk->tree.funs);

	fini_token (&awk->ntok);
	fini_token (&awk->tok);
	fini_token (&awk->ptok);

	if (awk->parse.incl_hist.ptr) hawk_freemem (awk, awk->parse.incl_hist.ptr);
	hawk_clearsionames (awk);

	/* destroy dynamically allocated options */
	for (i = 0; i < HAWK_COUNTOF(awk->opt.mod); i++)
	{
		if (awk->opt.mod[i].ptr) hawk_freemem (awk, awk->opt.mod[i].ptr);
	}

	if (awk->log.len > 0)
	{
		/* flush pending log message that could be generated by the fini 
		 * callbacks. however, the actual logging might not be produced at
		 * this point because one of the callbacks could arrange to stop
		 * logging */
		int shuterr = awk->shuterr;
		awk->shuterr = 1;
		awk->prm.logwrite (awk, awk->log.last_mask, awk->log.ptr, awk->log.len);
		awk->shuterr = shuterr;
	}

	if (awk->log.ptr) 
	{
		hawk_freemem (awk, awk->log.ptr);
		awk->log.capa = 0;
		awk->log.len = 0;
	}
}

static hawk_rbt_walk_t unload_module (hawk_rbt_t* rbt, hawk_rbt_pair_t* pair, void* ctx)
{
	hawk_t* awk = (hawk_t*)ctx;
	hawk_mod_data_t* md;

	md = HAWK_RBT_VPTR(pair);
	if (md->mod.unload) md->mod.unload (&md->mod, awk);
	if (md->handle) awk->prm.modclose (awk, md->handle);

	return HAWK_RBT_WALK_FORWARD;
}

void hawk_clear (hawk_t* awk)
{
	hawk_ecb_t* ecb;

	for (ecb = awk->ecb; ecb; ecb = ecb->next)
	{
		if (ecb->clear) ecb->clear (awk);
	}

	awk->haltall = 0;

	clear_token (&awk->tok);
	clear_token (&awk->ntok);
	clear_token (&awk->ptok);

	/* clear all loaded modules */
	hawk_rbt_walk (awk->modtab, unload_module, awk);
	hawk_rbt_clear (awk->modtab);

	HAWK_ASSERT (HAWK_ARR_SIZE(awk->parse.gbls) == awk->tree.ngbls);
	/* delete all non-builtin global variables */
	hawk_arr_delete (
		awk->parse.gbls, awk->tree.ngbls_base, 
		HAWK_ARR_SIZE(awk->parse.gbls) - awk->tree.ngbls_base);

	hawk_arr_clear (awk->parse.lcls);
	hawk_arr_clear (awk->parse.params);
	hawk_htb_clear (awk->parse.named);
	hawk_htb_clear (awk->parse.funs);

	awk->parse.nlcls_max = 0; 
	awk->parse.depth.block = 0;
	awk->parse.depth.loop = 0;
	awk->parse.depth.expr = 0;
	awk->parse.depth.incl = 0;
	awk->parse.pragma.trait = (awk->opt.trait & (HAWK_IMPLICIT | HAWK_MULTILINESTR | HAWK_STRIPRECSPC | HAWK_STRIPSTRSPC)); /* implicit on if you didn't mask it off in awk->opt.trait with hawk_setopt */
	awk->parse.pragma.rtx_stack_limit = 0;
	awk->parse.pragma.entry[0] = '\0';

	awk->parse.incl_hist.count =0;

	/* clear parse trees */
	/*awk->tree.ngbls_base = 0;
	awk->tree.ngbls = 0; */
	awk->tree.ngbls = awk->tree.ngbls_base;

	awk->tree.cur_fun.ptr = HAWK_NULL;
	awk->tree.cur_fun.len = 0;
	hawk_htb_clear (awk->tree.funs);

	if (awk->tree.begin) 
	{
		hawk_clrpt (awk, awk->tree.begin);
		awk->tree.begin = HAWK_NULL;
		awk->tree.begin_tail = HAWK_NULL;
	}

	if (awk->tree.end) 
	{
		hawk_clrpt (awk, awk->tree.end);
		awk->tree.end = HAWK_NULL;
		awk->tree.end_tail = HAWK_NULL;
	}

	while (awk->tree.chain) 
	{
		hawk_chain_t* next = awk->tree.chain->next;
		if (awk->tree.chain->pattern) hawk_clrpt (awk, awk->tree.chain->pattern);
		if (awk->tree.chain->action) hawk_clrpt (awk, awk->tree.chain->action);
		hawk_freemem (awk, awk->tree.chain);
		awk->tree.chain = next;
	}

	awk->tree.chain_tail = HAWK_NULL;
	awk->tree.chain_size = 0;

	/* this table must not be cleared here as there can be a reference
	 * to an entry of this table from errinf.loc.file when hawk_parse() 
	 * failed. this table is cleared in hawk_parse().
	 * hawk_claersionames (awk);
	 */
}

void hawk_getprm (hawk_t* hawk, hawk_prm_t* prm)
{
	*prm = hawk->prm;
}

void hawk_setprm (hawk_t* hawk, const hawk_prm_t* prm)
{
	hawk->prm = *prm;
}

static int dup_str_opt (hawk_t* hawk, const void* value, hawk_oocs_t* tmp)
{
	if (value)
	{
		tmp->ptr = hawk_dupoocstr(hawk, value, &tmp->len);
		if (HAWK_UNLIKELY(!tmp->ptr)) return -1;
	}
	else
	{
		tmp->ptr = HAWK_NULL;
		tmp->len = 0;
	}

	return 0;
}

int hawk_setopt (hawk_t* hawk, hawk_opt_t id, const void* value)
{
	switch (id)
	{
		case HAWK_OPT_TRAIT:
			hawk->opt.trait = *(const int*)value;
			return 0;

		case HAWK_OPT_MODLIBDIRS:
		case HAWK_OPT_MODPREFIX:
		case HAWK_OPT_MODPOSTFIX:
		{
			hawk_oocs_t tmp;
			int idx;

			if (dup_str_opt(hawk, value, &tmp) <= -1) return -1;

			idx = id - HAWK_OPT_MODLIBDIRS;
			if (hawk->opt.mod[idx].ptr) hawk_freemem (hawk, hawk->opt.mod[idx].ptr);

			hawk->opt.mod[idx] = tmp;
			return 0;
		}

		case HAWK_OPT_INCLUDEDIRS:
		{
			hawk_oocs_t tmp;
			if (dup_str_opt(hawk, value, &tmp) <= -1) return -1;
			if (hawk->opt.includedirs.ptr) hawk_freemem (hawk, hawk->opt.includedirs.ptr);
			hawk->opt.includedirs = tmp;
			return 0;
		}

		case HAWK_OPT_DEPTH_INCLUDE:
		case HAWK_OPT_DEPTH_BLOCK_PARSE:
		case HAWK_OPT_DEPTH_BLOCK_RUN:
		case HAWK_OPT_DEPTH_EXPR_PARSE:
		case HAWK_OPT_DEPTH_EXPR_RUN:
		case HAWK_OPT_DEPTH_REX_BUILD:
		case HAWK_OPT_DEPTH_REX_MATCH:
			hawk->opt.depth.a[id - HAWK_OPT_DEPTH_INCLUDE] = *(const hawk_oow_t*)value;
			return 0;

		case HAWK_OPT_RTX_STACK_LIMIT:
			hawk->opt.rtx_stack_limit = *(const hawk_oow_t*)value;
			if (hawk->opt.rtx_stack_limit < HAWK_MIN_RTX_STACK_LIMIT) hawk->opt.rtx_stack_limit = HAWK_MIN_RTX_STACK_LIMIT;
			else if (hawk->opt.rtx_stack_limit > HAWK_MAX_RTX_STACK_LIMIT) hawk->opt.rtx_stack_limit = HAWK_MAX_RTX_STACK_LIMIT;
			return 0;


		case HAWK_OPT_LOG_MASK:
			hawk->opt.log_mask = *(hawk_bitmask_t*)value;
			return 0;

		case HAWK_OPT_LOG_MAXCAPA:
			hawk->opt.log_maxcapa = *(hawk_oow_t*)value;
			return 0;

	}

	hawk_seterrnum (hawk, HAWK_NULL, HAWK_EINVAL);
	return -1;
}

int hawk_getopt (hawk_t* hawk, hawk_opt_t id, void* value)
{
	switch  (id)
	{
		case HAWK_OPT_TRAIT:
			*(int*)value = hawk->opt.trait;
			return 0;

		case HAWK_OPT_MODLIBDIRS:
		case HAWK_OPT_MODPREFIX:
		case HAWK_OPT_MODPOSTFIX:
			*(const hawk_ooch_t**)value = hawk->opt.mod[id - HAWK_OPT_MODLIBDIRS].ptr;
			return 0;

		case HAWK_OPT_INCLUDEDIRS:
			*(const hawk_ooch_t**)value = hawk->opt.includedirs.ptr;
			return 0;

		case HAWK_OPT_DEPTH_INCLUDE:
		case HAWK_OPT_DEPTH_BLOCK_PARSE:
		case HAWK_OPT_DEPTH_BLOCK_RUN:
		case HAWK_OPT_DEPTH_EXPR_PARSE:
		case HAWK_OPT_DEPTH_EXPR_RUN:
		case HAWK_OPT_DEPTH_REX_BUILD:
		case HAWK_OPT_DEPTH_REX_MATCH:
			*(hawk_oow_t*)value = hawk->opt.depth.a[id - HAWK_OPT_DEPTH_INCLUDE];
			return 0;

		case HAWK_OPT_RTX_STACK_LIMIT:
			*(hawk_oow_t*)value = hawk->opt.rtx_stack_limit;
			return 0;

		case HAWK_OPT_LOG_MASK:
			*(hawk_bitmask_t*)value = hawk->opt.log_mask;
			return 0;

		case HAWK_OPT_LOG_MAXCAPA:
			*(hawk_oow_t*)value = hawk->opt.log_maxcapa;
			return 0;

	};

	hawk_seterrnum (hawk, HAWK_NULL, HAWK_EINVAL);
	return -1;
}

void hawk_haltall (hawk_t* awk)
{
	awk->haltall = 1;
}

hawk_ecb_t* hawk_popecb (hawk_t* awk)
{
	hawk_ecb_t* top = awk->ecb;
	if (top) awk->ecb = top->next;
	return top;
}

void hawk_pushecb (hawk_t* awk, hawk_ecb_t* ecb)
{
	ecb->next = awk->ecb;
	awk->ecb = ecb;
}

/* ------------------------------------------------------------------------ */

hawk_oow_t hawk_fmttoucstr_ (hawk_t* hawk, hawk_uch_t* buf, hawk_oow_t bufsz, const hawk_uch_t* fmt, ...) 
{
	va_list ap;
	hawk_oow_t n;
	va_start(ap, fmt);
	n = hawk_gem_vfmttoucstr(hawk_getgem(hawk), buf, bufsz, fmt, ap);
	va_end(ap);
	return n; 
}

hawk_oow_t hawk_fmttobcstr_ (hawk_t* hawk, hawk_bch_t* buf, hawk_oow_t bufsz, const hawk_bch_t* fmt, ...) 
{
	va_list ap;
	hawk_oow_t n;
	va_start(ap, fmt);
	n = hawk_gem_vfmttobcstr(hawk_getgem(hawk), buf, bufsz, fmt, ap);
	va_end(ap);
	return n;
}


int hawk_buildrex (hawk_t* hawk, const hawk_ooch_t* ptn, hawk_oow_t len, hawk_tre_t** code, hawk_tre_t** icode)
{
	return hawk_gem_buildrex(hawk_getgem(hawk), ptn, len, !(hawk->opt.trait & HAWK_REXBOUND), code, icode);
}
