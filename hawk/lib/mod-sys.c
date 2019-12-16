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

#include "mod-sys.h"
#include "hawk-prv.h"

#if defined(_WIN32)
#	include <windows.h>
#	include <process.h>
#	include <tchar.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSEXCEPTIONS
#	define INCL_ERRORS
#	include <os2.h>
#elif defined(__DOS__)
#	include <dos.h>
#else
#	include "syscall.h"

#	define ENABLE_SYSLOG
#	include <syslog.h>

#	include <sys/socket.h>

#endif

#include <stdlib.h> /* getenv, system */
#include <time.h>
#include <errno.h>
#include <string.h>

#define DEFAULT_MODE (0777)

#define CLOSE_KEEPFD (1 << 0)

enum sys_rc_t
{
	RC_ERROR = -1,
	RC_ENOIMPL = -2,
	RC_ENOSYS = -3,
	RC_ENOMEM = -4,
	RC_EAGAIN = -5,
	RC_EINTR = -6,
	RC_EINVAL = -7,
	RC_ECHILD = -8,
	RC_EPERM = -9,
	RC_EBADF = -10,
	RC_ENOENT = -11,
	RC_EEXIST = -12,
	RC_ENOTDIR = -13
};
typedef enum sys_rc_t sys_rc_t;

/* ------------------------------------------------------------------------ */

typedef enum syslog_type_t syslog_type_t;
enum syslog_type_t
{
	SYSLOG_LOCAL,
	SYSLOG_REMOTE
};

struct mod_ctx_t
{
	hawk_rbt_t* rtxtab;

/*
	struct
	{
		syslog_type_t type;
		char* ident;
		hawk_skad_t skad;
		int syslog_opened; // has openlog() been called?
		int opt;
		int fac;
		int sck;
		hawk_becs_t* dmsgbuf;
	} log;
*/
};
typedef struct mod_ctx_t mod_ctx_t;

/* ------------------------------------------------------------------------ */

enum sys_node_data_type_t
{
	SYS_NODE_DATA_FD,
	SYS_NODE_DATA_DIR
};
typedef enum sys_node_data_type_t sys_node_data_type_t;

struct sys_node_data_t
{
	sys_node_data_type_t type;
	union
	{
		int fd;
		//hawk_dir_t* dir;
	} u;
};
typedef struct sys_node_data_t sys_node_data_t;

struct sys_list_data_t
{
	hawk_ooch_t errmsg[256];
	hawk_bch_t* readbuf;
	hawk_oow_t readbuf_capa;
};
typedef struct sys_list_data_t sys_list_data_t;

#define __IMAP_NODE_T_DATA sys_node_data_t ctx;
#define __IMAP_LIST_T_DATA sys_list_data_t ctx;
#define __IMAP_LIST_T sys_list_t
#define __IMAP_NODE_T sys_node_t
#define __MAKE_IMAP_NODE __new_sys_node
#define __FREE_IMAP_NODE __free_sys_node
#include "imap-imp.h"

struct rtx_data_t
{
	sys_list_t sys_list;
};
typedef struct rtx_data_t rtx_data_t;

/* ------------------------------------------------------------------------ */

static HAWK_INLINE sys_rc_t syserr_to_rc (int syserr)
{
	switch (syserr)
	{
	#if defined(EAGAIN) && defined(EWOULDBLOCK) && (EAGAIN == EWOULDBLOCK)
		case EAGAIN: return RC_EAGAIN;
	#elif defined(EAGAIN) && defined(EWOULDBLOCK) && (EAGAIN != EWOULDBLOCK)
		case EAGAIN: case EWOULDBLOCK: return RC_EAGAIN;
	#elif defined(EAGAIN)
		case EAGAIN: return RC_EAGAIN;
	#elif defined(EWOULDBLOCK)
		case EWOULDBLOCK: return RC_EAGAIN;
	#endif

		case EBADF: return RC_EBADF;
		case ECHILD: return RC_ECHILD;
		case EINTR:  return RC_EINTR;
		case EINVAL: return RC_EINVAL;
		case ENOMEM:  return RC_ENOMEM;
		case ENOSYS:  return RC_ENOSYS;
		case EPERM:  return RC_EPERM;

		default: return RC_ERROR;
	}
}

#if 0
static HAWK_INLINE sys_rc_t direrr_to_rc (hawk_dir_errnum_t direrr)
{
	switch (direrr)
	{
		case HAWK_DIR_ENOIMPL: return RC_ENOIMPL;
		case HAWK_DIR_ENOMEM: return RC_ENOMEM;
		case HAWK_DIR_EINVAL: return RC_EINVAL;
		case HAWK_DIR_EPERM: return RC_EPERM;
		case HAWK_DIR_ENOENT: return RC_ENOENT;
		case HAWK_DIR_EEXIST: return RC_EEXIST;
		case HAWK_DIR_ENOTDIR: return RC_ENOTDIR;
		case HAWK_DIR_EINTR: return RC_EINTR;
		case HAWK_DIR_EAGAIN: return RC_EAGAIN;

		default: return RC_ERROR;
	}
}

static HAWK_INLINE sys_rc_t awkerr_to_rc (hawk_dir_errnum_t awkerr)
{
	switch (awkerr)
	{
		case HAWK_ENOIMPL: return RC_ENOIMPL;
		case HAWK_ENOMEM: return RC_ENOMEM;
		case HAWK_EINVAL: return RC_EINVAL;
		case HAWK_EPERM: return RC_EPERM;
		case HAWK_ENOENT: return RC_ENOENT;
		case HAWK_EEXIST: return RC_EEXIST;

		default: return RC_ERROR;
	}
}
#endif

static const hawk_ooch_t* rc_to_errstr (sys_rc_t rc)
{
	switch (rc)
	{
		case RC_EAGAIN:  return HAWK_T("resource temporarily unavailable");
		case RC_EBADF:   return HAWK_T("bad file descriptor");
		case RC_ECHILD:  return HAWK_T("no child processes");
		case RC_EEXIST:  return HAWK_T("file exists");
		case RC_EINTR:   return HAWK_T("interrupted");
		case RC_EINVAL:  return HAWK_T("invalid argument");
		case RC_ENOENT:  return HAWK_T("no such file or directory");
		case RC_ENOIMPL: return HAWK_T("not implemented"); /* not implemented in this module */
		case RC_ENOMEM:  return HAWK_T("not enough space");
		case RC_ENOTDIR: return HAWK_T("not a directory");
		case RC_ENOSYS:  return HAWK_T("not implemented in system");
		case RC_EPERM:   return HAWK_T("operation not permitted");
		case RC_ERROR:   return HAWK_T("error");
		default:         return HAWK_T("unknown error");
	};
}

static void set_errmsg_on_sys_list (hawk_rtx_t* rtx, sys_list_t* sys_list, const hawk_ooch_t* errfmt, ...)
{
	if (errfmt)
	{
		va_list ap;
		va_start (ap, errfmt);
		hawk_strxvfmt (sys_list->ctx.errmsg, HAWK_COUNTOF(sys_list->ctx.errmsg), errfmt, ap);
		va_end (ap);
	}
	else
	{
		hawk_copy_oocstr (sys_list->ctx.errmsg, HAWK_COUNTOF(sys_list->ctx.errmsg), hawk_rtx_geterrmsg(rtx));
	}
}

static HAWK_INLINE void set_errmsg_on_sys_list_with_syserr (hawk_rtx_t* rtx, sys_list_t* sys_list)
{
	set_errmsg_on_sys_list (rtx, sys_list, HAWK_T("%hs"), strerror(errno));
}


/* ------------------------------------------------------------------------ */

static sys_node_t* new_sys_node_fd (hawk_rtx_t* rtx, sys_list_t* list, int fd)
{
	sys_node_t* node;

	node = __new_sys_node(rtx, list);
	if (!node) return HAWK_NULL;

	node->ctx.type = SYS_NODE_DATA_FD;
	node->ctx.u.fd = fd;
	return node;
}

static sys_node_t* new_sys_node_dir (hawk_rtx_t* rtx, sys_list_t* list, hawk_dir_t* dir)
{
	sys_node_t* node;

	node = __new_sys_node(rtx, list);
	if (!node) return HAWK_NULL;

	node->ctx.type = SYS_NODE_DATA_DIR;
	node->ctx.u.dir = dir;
	return node;
}

static void free_sys_node (hawk_rtx_t* rtx, sys_list_t* list, sys_node_t* node)
{
	switch (node->ctx.type)
	{
		case SYS_NODE_DATA_FD:
			if (node->ctx.u.fd >= 0) 
			{
				close (node->ctx.u.fd);
				node->ctx.u.fd = -1;
			}
			break;

		case  SYS_NODE_DATA_DIR:
			if (node->ctx.u.dir)
			{
				hawk_dir_close (node->ctx.u.dir);
				node->ctx.u.dir = HAWK_NULL;
			}
			break;
	}
	__free_sys_node (rtx, list, node);
}

/* ------------------------------------------------------------------------ */

static HAWK_INLINE sys_list_t* rtx_to_sys_list (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	mod_ctx_t* mctx = (mod_ctx_t*)fi->mod->ctx;
	hawk_rbt_pair_t* pair;

	pair = hawk_rbt_search(mctx->rtxtab, &rtx, HAWK_SIZEOF(rtx));
	HAWK_ASSERT (hawk_rtx_getawk(rtx), pair != HAWK_NULL);
	return (sys_list_t*)HAWK_RBT_VPTR(pair);
}

static HAWK_INLINE sys_node_t* get_sys_list_node (sys_list_t* sys_list, hawk_int_t id)
{
	if (id < 0 || id >= sys_list->map.high || !sys_list->map.tab[id]) return HAWK_NULL;
	return sys_list->map.tab[id];
}

/* ------------------------------------------------------------------------ */

static sys_node_t* get_sys_list_node_with_arg (hawk_rtx_t* rtx, sys_list_t* sys_list, hawk_val_t* arg)
{
	hawk_int_t id;
	sys_node_t* sys_node;

	if (hawk_rtx_valtoint(rtx, arg, &id) <= -1)
	{
		set_errmsg_on_sys_list (rtx, sys_list, HAWK_T("illegal instance id"));
		return HAWK_NULL;
	}
	else if (!(sys_node = get_sys_list_node(sys_list, id)))
	{
		set_errmsg_on_sys_list (rtx, sys_list, HAWK_T("invalid instance id - %zd"), (hawk_oow_t)id);
		return HAWK_NULL;
	}

	return sys_node;
}

/* ------------------------------------------------------------------------ */

static int fnc_errmsg (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	hawk_val_t* retv;

	sys_list = rtx_to_sys_list(rtx, fi);
	retv = hawk_rtx_makestrvalwithoocstr(rtx, sys_list->ctx.errmsg);
	if (!retv) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

/* ------------------------------------------------------------------------ */

static int fnc_close (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	sys_node_t* sys_node;
	int rx = RC_ERROR;
	hawk_int_t cflags;

	sys_list = rtx_to_sys_list(rtx, fi);
	sys_node = get_sys_list_node_with_arg(rtx, sys_list, hawk_rtx_getarg(rtx, 0));

	if (hawk_rtx_getnargs(rtx) >= 2 && (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 1), &cflags) <= -1 || cflags < 0)) cflags = 0;

	if (sys_node && sys_node->ctx.type == SYS_NODE_DATA_FD)
	{
		/* although free_sys_node can handle other types, sys::close() is allowed to
		 * close nodes of the SYS_NODE_DATA_FD type only */
		if (cflags & CLOSE_KEEPFD)  /* this flag applies to file descriptors only */
		{
			sys_node->ctx.u.fd = -1; /* you may leak the original file descriptor. */
		}

		free_sys_node (rtx, sys_list, sys_node);
		rx = 0;
	}
	else
	{
		rx = RC_EINVAL;
		set_errmsg_on_sys_list (rtx, sys_list, rc_to_errstr(rx));
	}

	hawk_rtx_setretval (rtx, hawk_rtx_makeintval(rtx, rx));
	return 0;
}

/*
  BEGIN {
     f = sys::open ("/tmp/test.txt", sys::O_RDONLY);
     while (sys::read(f, x, 10) > 0) printf (B"%s", x);
     sys::close (f);
  }
*/

static int fnc_open (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	
	hawk_int_t rx = RC_ERROR, oflags = 0, mode = DEFAULT_MODE;
	int fd;
	hawk_bch_t* pstr;
	hawk_oow_t plen;
	hawk_val_t* a0;

	sys_list = rtx_to_sys_list(rtx, fi);

	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 1), &oflags) <= -1 || oflags < 0) oflags = O_RDONLY;
	if (hawk_rtx_getnargs(rtx) >= 3 && (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 2), &mode) <= -1 || mode < 0)) mode = DEFAULT_MODE;

#if defined(O_LARGEFILE)
	oflags |= O_LARGEFILE;
#endif

	a0 = hawk_rtx_getarg(rtx, 0);
	pstr = hawk_rtx_getvalbcstr(rtx, a0, &plen);
	if (!pstr) goto fail;
	fd = open(pstr, oflags, mode);
	hawk_rtx_freevalbcstr (rtx, a0, pstr);

	if (fd >= 0)
	{
		sys_node_t* new_node;

		new_node = new_sys_node_fd(rtx, sys_list, fd);
		if (new_node) 
		{
			rx = new_node->id;
		}
		else 
		{
			close (fd);
		fail:
			rx = awkerr_to_rc(hawk_rtx_geterrnum(rtx));
			set_errmsg_on_sys_list (rtx, sys_list, HAWK_NULL);
		}
	}
	else
	{
		rx = syserr_to_rc(errno);
		set_errmsg_on_sys_list_with_syserr (rtx, sys_list);
	}

	HAWK_ASSERT (HAWK_IN_QUICKINT_RANGE(rx));
	hawk_rtx_setretval (rtx, hawk_rtx_makeintval(rtx, rx));
	return 0;
}

/*
	a = sys::openfd(1);
	sys::write (a, B"let me write something here\n");
	sys::close (a, sys::C_KEEPFD); ## set C_KEEPFD to release 1 without closing it.
	##sys::close (a);
	print "done\n";
*/

static int fnc_openfd (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	/* wrap a raw system file descriptor into the internal management node */

	sys_list_t* sys_list;
	hawk_int_t rx = RC_ERROR;
	hawk_int_t fd;

	sys_list = rtx_to_sys_list(rtx, fi);

	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 0), &fd) <= -1)
	{
		rx = awkerr_to_rc(hawk_rtx_geterrnum(rtx));
		set_errmsg_on_sys_list (rtx, sys_list, HAWK_NULL);
	}
	else if (fd >= 0 && fd <= HAWK_TYPE_MAX(int))
	{
		sys_node_t* sys_node;

		sys_node = new_sys_node_fd(rtx, sys_list, fd);
		if (sys_node) 
		{
			rx = sys_node->id;
		}
		else 
		{
			rx = awkerr_to_rc(hawk_rtx_geterrnum(rtx));
			set_errmsg_on_sys_list (rtx, sys_list, HAWK_NULL);
		}
	}
	else
	{
		rx = RC_EINVAL;
		set_errmsg_on_sys_list (rtx, sys_list, rc_to_errstr(rx));
	}

	HAWK_ASSERT (HAWK_IN_QUICKINT_RANGE(rx));
	hawk_rtx_setretval (rtx, hawk_rtx_makeintval(rtx, rx));
	return 0;
}


static int fnc_read (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	sys_node_t* sys_node;
	hawk_int_t rx = RC_ERROR;
	hawk_int_t reqsize = 8192;

	if (hawk_rtx_getnargs(rtx) >= 3 && (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 2), &reqsize) <= -1 || reqsize <= 0)) reqsize = 8192;
	if (reqsize > HAWK_QUICKINT_MAX) reqsize = HAWK_QUICKINT_MAX;

	sys_list = rtx_to_sys_list(rtx, fi);
	sys_node = get_sys_list_node_with_arg(rtx, sys_list, hawk_rtx_getarg(rtx, 0));
	if (sys_node && sys_node->ctx.type == SYS_NODE_DATA_FD)
	{
		if (reqsize > sys_list->ctx.readbuf_capa)
		{
			hawk_bch_t* tmp = hawk_rtx_reallocmem(rtx, sys_list->ctx.readbuf, reqsize);
			if (!tmp)
			{
				set_errmsg_on_sys_list (rtx, sys_list, HAWK_NULL);
				goto done;
			}
			sys_list->ctx.readbuf = tmp;
			sys_list->ctx.readbuf_capa = reqsize;
		}

		rx = read(sys_node->ctx.u.fd, sys_list->ctx.readbuf, reqsize);
		if (rx <= 0) 
		{
			if (rx <= -1) 
			{
				rx = syserr_to_rc(errno);
				set_errmsg_on_sys_list_with_syserr(rtx, sys_list);
			}
			goto done;
		}
		else
		{
			hawk_val_t* sv;
			int x;

			sv = hawk_rtx_makembsval(rtx, sys_list->ctx.readbuf, rx);
			if (!sv) return -1;

			hawk_rtx_refupval (rtx, sv);
			x = hawk_rtx_setrefval(rtx, (hawk_val_ref_t*)hawk_rtx_getarg(rtx, 1), sv);
			hawk_rtx_refdownval (rtx, sv);
			if (x <= -1) return -1;
		}
	}
	else 
	{
		rx = RC_EINVAL;
		set_errmsg_on_sys_list (rtx, sys_list, rc_to_errstr(rx));
	}

done:
	/* the value in 'rx' never exceeds HAWK_QUICKINT_MAX as 'reqsize' has been limited to
	 * it before the call to 'read'. so it's safe not to check the result of hawk_rtx_makeintval() */
	hawk_rtx_setretval (rtx, hawk_rtx_makeintval(rtx, rx));
	return 0;
}

static int fnc_write (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	sys_node_t* sys_node;
	hawk_int_t rx = RC_ERROR;

	sys_list = rtx_to_sys_list(rtx, fi);
	sys_node = get_sys_list_node_with_arg(rtx, sys_list, hawk_rtx_getarg(rtx, 0));
	if (sys_node && sys_node->ctx.type == SYS_NODE_DATA_FD)
	{
		hawk_bch_t* dptr;
		hawk_oow_t dlen;
		hawk_val_t* a1;

		a1 = hawk_rtx_getarg(rtx, 1);
		dptr = hawk_rtx_getvalbcstr(rtx, a1, &dlen);
		if (dptr)
		{
			rx = write(sys_node->ctx.u.fd, dptr, dlen);
			if (rx <= -1) 
			{
				rx = syserr_to_rc(errno);
				set_errmsg_on_sys_list_with_syserr(rtx, sys_list);
			}
			hawk_rtx_freevalbcstr (rtx, a1, dptr);
		}
		else
		{
			set_errmsg_on_sys_list (rtx, sys_list, HAWK_NULL);
		}
	}
	else
	{
		rx = RC_EINVAL;
		set_errmsg_on_sys_list (rtx, sys_list, rc_to_errstr(rx));
	}

	hawk_rtx_setretval (rtx, hawk_rtx_makeintval(rtx, rx));
	return 0;
}

/* ------------------------------------------------------------------------ */

/*
	a = sys::open("/etc/inittab", sys::O_RDONLY);
	x = sys::open("/etc/fstab", sys::O_RDONLY);

	b = sys::dup(a);
	sys::close(a);

	while (sys::read(b, abc, 100) > 0) printf (B"%s", abc);

	print "-------------------------------";

	c = sys::dup(x, b, sys::O_CLOEXEC);
	## assertion: b == c
	sys::close (x);

	while (sys::read(c, abc, 100) > 0) printf (B"%s", abc);
	sys::close (c);
*/

static int fnc_dup (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	sys_node_t* sys_node, * sys_node2 = HAWK_NULL;
	hawk_int_t rx = RC_ERROR;
	hawk_int_t oflags = 0;

	sys_list = rtx_to_sys_list(rtx, fi);
	sys_node = get_sys_list_node_with_arg(rtx, sys_list, hawk_rtx_getarg(rtx, 0));
	if (hawk_rtx_getnargs(rtx) >= 2)
	{
		sys_node2 = get_sys_list_node_with_arg(rtx, sys_list, hawk_rtx_getarg(rtx, 1));
		if (!sys_node2 || sys_node2->ctx.type != SYS_NODE_DATA_FD) goto fail_einval;
		if (hawk_rtx_getnargs(rtx) >= 3 && (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 2), &oflags) <= -1 || oflags < 0)) oflags = 0;
	}

	if (sys_node && sys_node->ctx.type == SYS_NODE_DATA_FD)
	{
		int fd;

		if (sys_node2)
		{
		#if defined(HAVE_DUP3)
			fd = dup3(sys_node->ctx.u.fd, sys_node2->ctx.u.fd, oflags);
		#else
			fd = dup2(sys_node->ctx.u.fd, sys_node2->ctx.u.fd);
		#endif
			if (fd >= 0)
			{
		#if defined(HAVE_DUP3)
				/* nothing extra for dup3 */
		#else
				if (oflags)
				{
					int nflags = 0;
				#if defined(O_CLOEXEC) && defined(FD_CLOEXEC)
					if (oflags & O_CLOEXEC) nflags |= FD_CLOEXEC;
				#endif
				#if defined(O_NONBLOCK) && defined(FD_NONBLOCK)
					/*if (oflags & O_NONBLOCK) nflags |= FD_NONBLOCK;  dup3() doesn't seem to support NONBLOCK. */
				#endif
					if (nflags) fcntl (fd, F_SETFD, nflags);
				
				}
		#endif
				sys_node2->ctx.u.fd = fd; /* dup2 or dup3 closes the descriptor implicitly */
				rx = sys_node2->id;
			}
			else
			{
				rx = syserr_to_rc(errno);
				set_errmsg_on_sys_list_with_syserr (rtx, sys_list);
			}
		}
		else
		{
			fd = dup(sys_node->ctx.u.fd);
			if (fd >= 0)
			{
				sys_node_t* new_node;

				new_node = new_sys_node_fd(rtx, sys_list, fd);
				if (new_node) 
				{
					rx = new_node->id;
				}
				else 
				{
					close (fd);
					rx = awkerr_to_rc(hawk_rtx_geterrnum(rtx));
					set_errmsg_on_sys_list (rtx, sys_list, HAWK_NULL);
				}
			}
			else
			{
				rx = syserr_to_rc(errno);
				set_errmsg_on_sys_list_with_syserr (rtx, sys_list);
			}
		}
	}
	else
	{
	fail_einval:
		rx = RC_EINVAL;
		set_errmsg_on_sys_list (rtx, sys_list, rc_to_errstr(rx));
	}

	hawk_rtx_setretval (rtx, hawk_rtx_makeintval(rtx, rx));
	return 0;
}

/* ------------------------------------------------------------------------ */

/*
	##if (sys::pipe(p0, p1) <= -1)
	if (sys::pipe(p0, p1, sys::O_NONBLOCK | sys::O_CLOEXEC) <= -1)
	{
		 print "pipe error";
		 return -1;
	}
	a = sys::fork();
	if (a <= -1)
	{
		print "fork error";
		sys::close (p0);
		sys::close (p1);
	}
	else if (a == 0)
	{
		## child
		printf ("child.... %d %d %d\n", sys::getpid(), p0, p1);
		sys::close (p1);
		while (1)
		{
			n = sys::read (p0, k, 3);
			if (n <= 0)
			{
				if (n == sys::RC_EAGAIN) continue; ## nonblock but data not available
				if (n != 0) print "ERROR: " sys::errmsg();
				break;
			}
			print k;
		}
		sys::close (p0);
	}
	else
	{
		## parent
		printf ("parent.... %d %d %d\n", sys::getpid(), p0, p1);
		sys::close (p0);
		sys::write (p1, B"hello");
		sys::write (p1, B"world");
		sys::close (p1);
		sys::wait(a);
	}
*/

static int fnc_pipe (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	/* create low-level pipes */

	sys_list_t* sys_list;
	int rx = RC_ERROR;
	int fds[2];
	hawk_int_t flags = 0;

	sys_list = rtx_to_sys_list(rtx, fi);

	if (hawk_rtx_getnargs(rtx) >= 3 && (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 2), &flags) <= -1 || flags < 0)) flags = 0;

#if defined(HAVE_PIPE2)
	if (pipe2(fds, flags) >= 0)
#else
	if (pipe(fds) >= 0)
#endif
	{
		sys_node_t* node1, * node2;

	#if defined(HAVE_PIPE2)
		/* do nothing extra */
	#else
		if (flags > 0)
		{
			int nflags = 0;

			/* needs translation from O_XXXX to FD_XXXX */
		#if defined(O_CLOEXEC) && defined(FD_CLOEXEC)
			if (flags & O_CLOEXEC) nflags |= FD_CLOEXEC;
		#endif
		#if defined(O_NONBLOCK) && defined(FD_NONBLOCK)
			if (flags & O_NONBLOCK) nflags |= FD_NONBLOCK;
		#endif

			if (nflags > 0)
			{
				fcntl (fds[0], F_SETFD, nflags);
				fcntl (fds[1], F_SETFD, nflags);
			}
		}
	#endif
		node1 = new_sys_node_fd(rtx, sys_list, fds[0]);
		node2 = new_sys_node_fd(rtx, sys_list, fds[1]);
		if (node1 && node2)
		{
			hawk_val_t* v;
			int x;

			v = hawk_rtx_makeintval(rtx, node1->id);
			if (!v) 
			{
			fail:
				free_sys_node (rtx, sys_list, node1);
				free_sys_node (rtx, sys_list, node2);
				return -1;
			}
			hawk_rtx_refupval (rtx, v);
			x = hawk_rtx_setrefval (rtx, (hawk_val_ref_t*)hawk_rtx_getarg(rtx, 0), v);
			hawk_rtx_refdownval (rtx, v);
			if (x <= -1) goto fail;

			v = hawk_rtx_makeintval(rtx, node2->id);
			if (!v) goto fail;
			hawk_rtx_refupval (rtx, v);
			x = hawk_rtx_setrefval (rtx, (hawk_val_ref_t*)hawk_rtx_getarg(rtx, 1), v);
			hawk_rtx_refdownval (rtx, v);
			if (x <= -1) goto fail;

			rx = 0;
		}
		else
		{
			set_errmsg_on_sys_list (rtx, sys_list, HAWK_NULL);
			if (node2) free_sys_node (rtx, sys_list, node2);
			if (node1) free_sys_node (rtx, sys_list, node1);
		}
	}
	else
	{
		rx = syserr_to_rc(errno);
		set_errmsg_on_sys_list_with_syserr (rtx, sys_list);
	}

	hawk_rtx_setretval (rtx, hawk_rtx_makeintval(rtx, rx));
	return 0;
}


/* ------------------------------------------------------------------------ */

#if 0
/*
        d = sys::opendir("/etc", sys::DIR_SORT);
        if (d >= 0)
        {
                while (sys::readdir(d,a) > 0) print a;
                sys::closedir(d);
        }
*/

static int fnc_opendir (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	sys_node_t* sys_node = HAWK_NULL;
	hawk_int_t rx = RC_ERROR, flags = 0;
	hawk_ooch_t* pstr;
	hawk_oow_t plen;
	hawk_val_t* a0;
	hawk_dir_t* dir;
	hawk_dir_errnum_t oe;

	sys_list = rtx_to_sys_list(rtx, fi);

	if (hawk_rtx_getnargs(rtx) >= 2 && (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 1), &flags) <= -1 || flags < 0)) flags = 0;

	a0 = hawk_rtx_getarg(rtx, 0);
	pstr = hawk_rtx_getvaloocstr(rtx, a0, &plen);
	if (!pstr) goto fail;
	dir = hawk_dir_open(hawk_rtx_getmmgr(rtx), 0, pstr, flags, &oe);
	hawk_rtx_freevaloocstr (rtx, a0, pstr);

	if (dir)
	{
		sys_node = new_sys_node_dir(rtx, sys_list, dir);
		if (sys_node) 
		{
			rx = sys_node->id;
		}
		else 
		{
			hawk_dir_close(dir);
		fail:
			set_errmsg_on_sys_list (rtx, sys_list, HAWK_NULL);
		}
	}
	else
	{
		rx = direrr_to_rc(oe);
		set_errmsg_on_sys_list (rtx, sys_list, rc_to_errstr(rx));
	}

	HAWK_ASSERT (HAWK_IN_QUICKINT_RANGE(rx));
	hawk_rtx_setretval (rtx, hawk_rtx_makeintval(rtx, rx));
	return 0;
}

static int fnc_closedir (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	sys_node_t* sys_node;
	int rx = RC_ERROR;

	sys_list = rtx_to_sys_list(rtx, fi);
	sys_node = get_sys_list_node_with_arg(rtx, sys_list, hawk_rtx_getarg(rtx, 0));
	if (sys_node && sys_node->ctx.type == SYS_NODE_DATA_DIR)
	{
		/* although free_sys_node() can handle other types, sys::closedir() is allowed to
		 * close nodes of the SYS_NODE_DATA_DIR type only */
		free_sys_node (rtx, sys_list, sys_node);
		rx = 0;
	}
	else
	{
		rx = RC_EINVAL;
		set_errmsg_on_sys_list (rtx, sys_list, rc_to_errstr(rx));
	}

	hawk_rtx_setretval (rtx, hawk_rtx_makeintval(rtx, rx));
	return 0;
}

static int fnc_readdir (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	sys_node_t* sys_node;
	hawk_int_t rx = RC_ERROR;

	sys_list = rtx_to_sys_list(rtx, fi);
	sys_node = get_sys_list_node_with_arg(rtx, sys_list, hawk_rtx_getarg(rtx, 0));
	if (sys_node && sys_node->ctx.type == SYS_NODE_DATA_DIR)
	{
		int y;
		hawk_dir_ent_t ent;
		hawk_val_t* tmp;

		y = hawk_dir_read(sys_node->ctx.u.dir, &ent);
		if (y <= -1) 
		{
			rx = direrr_to_rc(hawk_dir_geterrnum(sys_node->ctx.u.dir));
			set_errmsg_on_sys_list (rtx, sys_list, rc_to_errstr(rx));
		}
		else if (y == 0) 
		{
			rx = 0; /* no more entry */
		}
		else
		{
			tmp = hawk_rtx_makestrvalwithoocstr(rtx, ent.name);
			if (!tmp)
			{
				rx = awkerr_to_rc(hawk_rtx_geterrnum(rtx));
				set_errmsg_on_sys_list (rtx, sys_list, rc_to_errstr(rx));
			}
			else
			{
				int n;
				hawk_rtx_refupval (rtx, tmp);
				n = hawk_rtx_setrefval (rtx, (hawk_val_ref_t*)hawk_rtx_getarg(rtx, 1), tmp);
				hawk_rtx_refdownval (rtx, tmp);
				if (n <= -1) return -1;

				rx = 1; /* has entry */
			}
		}
	}
	else 
	{
		rx = RC_EINVAL;
		set_errmsg_on_sys_list (rtx, sys_list, rc_to_errstr(rx));
	}

	/* the value in 'rx' never exceeds HAWK_QUICKINT_MAX as 'reqsize' has been limited to
	 * it before the call to 'read'. so it's safe not to check the result of hawk_rtx_makeintval() */
	hawk_rtx_setretval (rtx, hawk_rtx_makeintval(rtx, rx));
	return 0;
}
#endif
/* ------------------------------------------------------------------------ */

static int fnc_fork (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t pid;
	hawk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	pid = RC_ENOIMPL;
	set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(pid));
#elif defined(__OS2__)
	/* TOOD: implement this*/
	pid = RC_ENOIMPL;
	set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(pid));
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = RC_ENOIMPL;
	set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(pid));
#else
	pid = fork();
	if (pid <= -1) 
	{
		pid = syserr_to_rc(errno);
		set_errmsg_on_sys_list_with_syserr (rtx, rtx_to_sys_list(rtx, fi));
	}
#endif

	retv = hawk_rtx_makeintval(rtx, pid);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wait (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t pid;
	hawk_val_t* retv;
	int rx;
	hawk_oow_t nargs;
	hawk_int_t opts = 0;
	int status;

	nargs = hawk_rtx_getnargs(rtx);
	if (nargs >= 3)
	{
		if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 2), &opts) <= -1) return -1;
	}

	rx = hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 0), &pid);
	if (rx >= 0)
	{
#if defined(_WIN32)
		/* TOOD: implement this*/
		rx = RC_ENOIMPL;
		set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(rx));
		status = 0;
#elif defined(__OS2__)
		/* TOOD: implement this*/
		rx = RC_ENOIMPL;
		set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(rx));
		status = 0;
#elif defined(__DOS__)
		/* TOOD: implement this*/
		rx = RC_ENOIMPL;
		set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(rx));
		status = 0;
#else
		rx = waitpid(pid, &status, opts);
		if (rx <= -1) 
		{
			rx = syserr_to_rc(errno);
			set_errmsg_on_sys_list_with_syserr (rtx, rtx_to_sys_list(rtx, fi));
		}
#endif
	}

	retv = hawk_rtx_makeintval(rtx, rx);
	if (!retv) return -1;

	if (nargs >= 2)
	{
		hawk_val_t* sv;
		int x;

		sv = hawk_rtx_makeintval(rtx, status);
		if (!sv) return -1;

		hawk_rtx_refupval (rtx, sv);
		x = hawk_rtx_setrefval(rtx, (hawk_val_ref_t*)hawk_rtx_getarg(rtx, 1), sv);
		hawk_rtx_refdownval (rtx, sv);
		if (x <= -1)  
		{
			hawk_rtx_freemem (rtx, retv);
			return -1;
		}
	}

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wifexited (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t wstatus;
	hawk_val_t* retv;
	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 0), &wstatus) <= -1) return -1;
	retv = hawk_rtx_makeintval(rtx, WIFEXITED(wstatus));
	if (!retv) return -1;
	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wexitstatus (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t wstatus;
	hawk_val_t* retv;
	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 0), &wstatus) <= -1) return -1;
	retv = hawk_rtx_makeintval(rtx, WEXITSTATUS(wstatus));
	if (!retv) return -1;
	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wifsignaled (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t wstatus;
	hawk_val_t* retv;
	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 0), &wstatus) <= -1) return -1;
	retv = hawk_rtx_makeintval(rtx, WIFSIGNALED(wstatus));
	if (!retv) return -1;
	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wtermsig (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t wstatus;
	hawk_val_t* retv;
	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 0), &wstatus) <= -1) return -1;
	retv = hawk_rtx_makeintval(rtx, WTERMSIG(wstatus));
	if (!retv) return -1;
	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wcoredump (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t wstatus;
	hawk_val_t* retv;
	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 0), &wstatus) <= -1) return -1;
	retv = hawk_rtx_makeintval(rtx, WCOREDUMP(wstatus));
	if (!retv) return -1;
	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_kill (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t pid, sig;
	hawk_val_t* retv;
	int rx;

	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg (rtx, 0), &pid) <= -1 ||
	    hawk_rtx_valtoint(rtx, hawk_rtx_getarg (rtx, 1), &sig) <= -1)
	{
		rx = RC_ERROR;
	}
	else
	{
#if defined(_WIN32)
		/* TOOD: implement this*/
		rx = RC_ENOIMPL;
		set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(rx));
#elif defined(__OS2__)
		/* TOOD: implement this*/
		rx = RC_ENOIMPL;
		set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(rx));
#elif defined(__DOS__)
		/* TOOD: implement this*/
		rx = RC_ENOIMPL;
		set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(rx));
#else
		rx = kill(pid, sig);
		if (rx <= -1) 
		{
			rx = syserr_to_rc(errno);
			set_errmsg_on_sys_list_with_syserr (rtx, rtx_to_sys_list(rtx, fi));
		}
#endif
	}

	retv = hawk_rtx_makeintval(rtx, rx);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getpgid (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t pid;
	hawk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	pid = RC_ENOIMPL;
	set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(pid));
#elif defined(__OS2__)
	/* TOOD: implement this*/
	pid = RC_ENOIMPL;
	set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(pid));
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = RC_ENOIMPL;
	set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(pid));
#else
	/* TODO: support specifing calling process id other than 0 */
	#if defined(HAVE_GETPGID)
	pid = getpgid(0);
	if (pid <= -1) set_errmsg_on_sys_list_with_syserr (rtx, rtx_to_sys_list(rtx, fi));
	#elif defined(HAVE_GETPGRP)
	pid = getpgrp();
	if (pid <= -1) set_errmsg_on_sys_list_with_syserr (rtx, rtx_to_sys_list(rtx, fi));
	#else
	pid = RC_ENOIMPL;
	set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(pid));
	#endif
#endif

	retv = hawk_rtx_makeintval(rtx, pid);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getpid (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t pid;
	hawk_val_t* retv;

#if defined(_WIN32)
	pid = GetCurrentProcessId();

#elif defined(__OS2__)
	PTIB tib;
	PPIB pib;

	pid = (DosGetInfoBlocks(&tib, &pib) == NO_ERROR)? pib->pib_ulpid: -1;

#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = RC_ENOIMPL;
	set_errmsg_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), rc_to_errstr(pid));

#else
	pid = getpid ();
	/* getpid() never fails */
#endif

	retv = hawk_rtx_makeintval (rtx, pid);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_gettid (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_intptr_t pid;
	hawk_val_t* retv;

#if defined(_WIN32)
	pid = GetCurrentThreadId();
	
#elif defined(__OS2__)
	PTIB tib;
	PPIB pib;

	pid = (DosGetInfoBlocks (&tib, &pib) == NO_ERROR && tib->tib_ptib2)?
		 tib->tib_ptib2->tib2_ultid: -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = -1;

#else
	#if defined(SYS_gettid) && defined(HAWK_SYSCALL0)
	HAWK_SYSCALL0 (pid, SYS_gettid);
	#elif defined(SYS_gettid)
	pid = syscall(SYS_gettid);
	#else
	pid = -1;
	#endif
#endif

	retv = hawk_rtx_makeintval (rtx, (hawk_int_t)pid);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getppid (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t pid;
	hawk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	pid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	pid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = -1;

#else
	pid = getppid ();
#endif

	retv = hawk_rtx_makeintval (rtx, pid);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getuid (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t uid;
	hawk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	uid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	uid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	uid = -1;

#else
	uid = getuid ();
#endif

	retv = hawk_rtx_makeintval (rtx, uid);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getgid (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t gid;
	hawk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	gid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	gid = -1;

#elif defined(__DOS__)
	/* TOOD: implement this*/
	gid = -1;

#else
	gid = getgid ();
#endif

	retv = hawk_rtx_makeintval (rtx, gid);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_geteuid (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t uid;
	hawk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	uid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	uid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	uid = -1;

#else
	uid = geteuid ();
#endif

	retv = hawk_rtx_makeintval (rtx, uid);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getegid (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t gid;
	hawk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	gid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	gid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	gid = -1;

#else
	gid = getegid ();
#endif

	retv = hawk_rtx_makeintval (rtx, gid);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_sleep (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_int_t lv;
	hawk_flt_t fv;
	hawk_val_t* retv;
	int rx;

	rx = hawk_rtx_valtonum(rtx, hawk_rtx_getarg (rtx, 0), &lv, &fv);
	if (rx == 0)
	{
#if defined(_WIN32)
		Sleep ((DWORD)HAWK_SEC_TO_MSEC(lv));
		rx = 0;
#elif defined(__OS2__)
		DosSleep ((ULONG)HAWK_SEC_TO_MSEC(lv));
		rx = 0;
#elif defined(__DOS__)
		#if (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
			sleep (lv);
			rx = 0;
		#else
			rx = sleep (lv);
		#endif
#elif defined(HAVE_NANOSLEEP)
		struct timespec req;
		req.tv_sec = lv;
		req.tv_nsec = 0;
		rx = nanosleep(&req, HAWK_NULL);
#else
		rx = sleep(lv);
#endif
	}
	else if (rx >= 1)
	{
#if defined(_WIN32)
		Sleep ((DWORD)HAWK_SEC_TO_MSEC(fv));
		rx = 0;
#elif defined(__OS2__)
		DosSleep ((ULONG)HAWK_SEC_TO_MSEC(fv));
		rx = 0;
#elif defined(__DOS__)
		/* no high-resolution sleep() is available */
		#if (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
			sleep ((hawk_int_t)fv);
			rx = 0;
		#else
			rx = sleep((hawk_int_t)fv);
		#endif;
#elif defined(HAVE_NANOSLEEP)
		struct timespec req;
		req.tv_sec = (hawk_int_t)fv;
		req.tv_nsec = HAWK_SEC_TO_NSEC(fv - req.tv_sec);
		rx = nanosleep(&req, HAWK_NULL);
#elif defined(HAVE_SELECT)
		struct timeval req;
		req.tv_sec = (hawk_int_t)fv;
		req.tv_usec = HAWK_SEC_TO_USEC(fv - req.tv_sec);
		rx = select(0, HAWK_NULL, HAWK_NULL, HAWK_NULL, &req);
#else
		/* no high-resolution sleep() is available */
		rx = sleep((hawk_int_t)fv);
#endif
	}

	retv = hawk_rtx_makeintval(rtx, rx);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_gettime (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_val_t* retv;
	hawk_ntime_t now;

	if (hawk_get_time (&now) <= -1) now.sec = 0;

	retv = hawk_rtx_makeintval(rtx, now.sec);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_settime (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_val_t* retv;
	hawk_ntime_t now;
	hawk_int_t tmp;
	int rx;

	now.nsec = 0;

	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg (rtx, 0), &tmp) <= -1) rx = -1;
	else
	{
		now.sec = tmp;
		if (hawk_set_time(&now) <= -1) rx = -1;
		else rx = 0;
	}

	retv = hawk_rtx_makeintval (rtx, rx);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_mktime (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_ntime_t nt;
	hawk_oow_t nargs;
	hawk_val_t* retv;

	nargs = hawk_rtx_getnargs(rtx);
	if (nargs >= 1)
	{
		int sign;
		hawk_ooch_t* str, * p, * end;
		hawk_oow_t len;
		hawk_val_t* a0;
		hawk_btime_t bt;

		a0 = hawk_rtx_getarg(rtx, 0);
		str = hawk_rtx_getvaloocstr(rtx, a0, &len);
		if (str == HAWK_NULL) return -1;

		/* the string must be of the format  YYYY MM DD HH MM SS[ DST] */
		p = str;
		end = str + len;
		HAWK_MEMSET (&bt, 0, HAWK_SIZEOF(bt));

		sign = 1;
		if (p < end && *p == HAWK_T('-')) { sign = -1; p++; }
		while (p < end && hawk_is_ooch_digit(*p)) bt.year = bt.year * 10 + (*p++ - HAWK_T('0'));
		bt.year *= sign;
		bt.year -= 1900;
		while (p < end && (hawk_is_ooch_space(*p) || *p == HAWK_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == HAWK_T('-')) { sign = -1; p++; }
		while (p < end && hawk_is_ooch_digit(*p)) bt.mon = bt.mon * 10 + (*p++ - HAWK_T('0'));
		bt.mon *= sign;
		bt.mon -= 1;
		while (p < end && (hawk_is_ooch_space(*p) || *p == HAWK_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == HAWK_T('-')) { sign = -1; p++; }
		while (p < end && hawk_is_ooch_digit(*p)) bt.mday = bt.mday * 10 + (*p++ - HAWK_T('0'));
		bt.mday *= sign;
		while (p < end && (hawk_is_ooch_space(*p) || *p == HAWK_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == HAWK_T('-')) { sign = -1; p++; }
		while (p < end && hawk_is_ooch_digit(*p)) bt.hour = bt.hour * 10 + (*p++ - HAWK_T('0'));
		bt.hour *= sign;
		while (p < end && (hawk_is_ooch_space(*p) || *p == HAWK_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == HAWK_T('-')) { sign = -1; p++; }
		while (p < end && hawk_is_ooch_digit(*p)) bt.min = bt.min * 10 + (*p++ - HAWK_T('0'));
		bt.min *= sign;
		while (p < end && (hawk_is_ooch_space(*p) || *p == HAWK_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == HAWK_T('-')) { sign = -1; p++; }
		while (p < end && hawk_is_ooch_digit(*p)) bt.sec = bt.sec * 10 + (*p++ - HAWK_T('0'));
		bt.sec *= sign;
		while (p < end && (hawk_is_ooch_space(*p) || *p == HAWK_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == HAWK_T('-')) { sign = -1; p++; }
		while (p < end && hawk_is_ooch_digit(*p)) bt.isdst = bt.isdst * 10 + (*p++ - HAWK_T('0'));
		bt.isdst *= sign;
		while (p < end && (hawk_is_ooch_space(*p) || *p == HAWK_T('\0'))) p++;

		hawk_rtx_freevaloocstr (rtx, a0, str);
		hawk_timelocal (&bt, &nt);
	}
	else
	{
		/* get the current time when no argument is given */
		hawk_get_time (&nt);
	}

	retv = hawk_rtx_makeintval(rtx, nt.sec);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}


#define STRFTIME_UTC (1 << 0)

static int fnc_strftime (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{

	/*
	sys::strftime("%Y-%m-%d %H:%M:%S %z", sys::gettime()); 
	sys::strftime("%Y-%m-%d %H:%M:%S %z", sys::gettime(), sys::STRFTIME_UTC);
	*/

	hawk_bch_t* fmt;
	hawk_oow_t len;
	hawk_val_t* retv;

	fmt = hawk_rtx_valtobcstrdup(rtx, hawk_rtx_getarg(rtx, 0), &len);
	if (fmt) 
	{
		hawk_ntime_t nt;
		hawk_btime_t bt;
		hawk_int_t tmpsec, flags = 0;

		nt.nsec = 0;
		if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 1), &tmpsec) <= -1) 
		{
			nt.sec = 0;
		}
		else
		{
			nt.sec = tmpsec;
		}

		
		if (hawk_rtx_getnargs(rtx) >= 3 && (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 2), &flags) <= -1 || flags < 0)) flags = 0;

		if (((flags & STRFTIME_UTC)? hawk_gmtime(&nt, &bt): hawk_localtime(&nt, &bt)) >= 0)
		{
			hawk_bch_t tmpbuf[64], * tmpptr;
			struct tm tm;
			hawk_oow_t sl;

			HAWK_MEMSET (&tm, 0, HAWK_SIZEOF(tm));
			tm.tm_year = bt.year;
			tm.tm_mon = bt.mon;
			tm.tm_mday = bt.mday;
			tm.tm_hour = bt.hour;
			tm.tm_min = bt.min;
			tm.tm_sec = bt.sec;
			tm.tm_isdst = bt.isdst;
		#if defined(HAVE_STRUCT_TM_TM_GMTOFF)
			tm.tm_gmtoff = bt.gmtoff;
		#elif defined(HAVE_STRUCT_TM___TM_GMTOFF)
			tm.__tm_gmtoff = bt.gmtoff;
		#endif
			if (flags & STRFTIME_UTC)
			{
			#if defined(HAVE_STRUCT_TM_TM_ZONE)
				tm.tm_zone = "GMT";
			#elif defined(HAVE_STRUCT_TM___TM_ZONE)
				tm.__tm_zone = "GMT";
			#endif
			}

			sl = strftime(tmpbuf, HAWK_COUNTOF(tmpbuf), fmt, &tm);
			if (sl <= 0 || sl >= HAWK_COUNTOF(tmpbuf))
			{
				/* buffer too small */
				hawk_bch_t* tmp;
				hawk_oow_t tmpcapa, i, count = 0;

/*
man strftime >>>

RETURN VALUE
       The strftime() function returns the number of bytes placed in the array s, not including the  terminating  null  byte,  provided  the
       string,  including  the  terminating  null  byte,  fits.  Otherwise, it returns 0, and the contents of the array is undefined.  (This
       behavior applies since at least libc 4.4.4; very old versions of libc, such as libc 4.4.1, would return max  if  the  array  was  too
       small.)

       Note that the return value 0 does not necessarily indicate an error; for example, in many locales %p yields an empty string.
 
--------------------------------------------------------------------------------------
* 
I use 'count' to limit the maximum number of retries when 0 is returned.
*/

				for (i = 0; i < len;)
				{
					if (fmt[i] == HAWK_BT('%')) 
					{
						count++; /* the nubmer of % specifier */
						i++;
						if (i < len) i++;
					}
					else i++;
				}

				tmpptr = HAWK_NULL;
				tmpcapa = HAWK_COUNTOF(tmpbuf);
				if (tmpcapa < len) tmpcapa = len;

				do
				{
					if (count <= 0) 
					{
						if (tmpptr) hawk_rtx_freemem (rtx, tmpptr);
						tmpbuf[0] = HAWK_BT('\0');
						tmpptr = tmpbuf;
						break;
					}
					count--;

					tmpcapa *= 2;
					tmp = (hawk_bch_t*)hawk_rtx_reallocmem(rtx, tmpptr, tmpcapa * HAWK_SIZEOF(*tmpptr));
					if (!tmp) 
					{
						if (tmpptr) hawk_rtx_freemem (rtx, tmpptr);
						tmpbuf[0] = HAWK_BT('\0');
						tmpptr = tmpbuf;
						break;
					}

					tmpptr = tmp;
					sl = strftime(tmpptr, tmpcapa, fmt, &tm);
				}
				while (sl <= 0 || sl >= tmpcapa);
			}
			else
			{
				tmpptr = tmpbuf;
			}
			hawk_rtx_freemem (rtx, fmt);

			retv = hawk_rtx_makestrvalwithbcstr(rtx, tmpptr);
			if (tmpptr && tmpptr != tmpbuf) hawk_rtx_freemem (rtx, tmpptr);
			if (retv == HAWK_NULL) return -1;

			hawk_rtx_setretval (rtx, retv);
		}
		else
		{
			hawk_rtx_freemem (rtx, fmt);
		}
	}
	return 0;
}

static int fnc_getenv (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_bch_t* var;
	hawk_oow_t len;
	hawk_val_t* retv;

	var = hawk_rtx_valtobcstrdup(rtx, hawk_rtx_getarg (rtx, 0), &len);
	if (var)
	{
		hawk_bch_t* val;

		val = getenv(var);	
		if (val) 
		{
			retv = hawk_rtx_makestrvalwithbcstr(rtx, val);
			if (retv == HAWK_NULL) return -1;

			hawk_rtx_setretval (rtx, retv);
		}

		hawk_rtx_freemem (rtx, var);
	}

	return 0;
}

static int fnc_getnwifcfg (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_nwifcfg_t cfg;
	hawk_rtx_valtostr_out_t out;
	int ret = -1;

	out.type = HAWK_RTX_VALTOSTR_CPLCPY;
	out.u.cplcpy.ptr = cfg.name;
	out.u.cplcpy.len = HAWK_COUNTOF(cfg.name);
	if (hawk_rtx_valtostr(rtx, hawk_rtx_getarg(rtx, 0), &out) >= 0)
	{
		hawk_int_t type;
		int rx;

		rx = hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 1), &type);
		if (rx >= 0)
		{
			cfg.type = type;

			if (hawk_getnwifcfg(&cfg) >= 0)
			{
				/* make a map value containg configuration */
				hawk_int_t index, mtu;
				hawk_ooch_t addr[128];
				hawk_ooch_t mask[128];
				hawk_ooch_t ethw[32];
				hawk_val_map_data_t md[7];
				hawk_val_t* tmp;

				HAWK_MEMSET (md, 0, HAWK_SIZEOF(md));

				md[0].key.ptr = HAWK_T("index");
				md[0].key.len = 5;
				md[0].type = HAWK_VAL_MAP_DATA_INT;
				index = cfg.index;
				md[0].vptr = &index;

				md[1].key.ptr = HAWK_T("mtu");
				md[1].key.len = 3;
				md[1].type = HAWK_VAL_MAP_DATA_INT;
				mtu = cfg.mtu;
				md[1].vptr = &mtu;
		
				md[2].key.ptr = HAWK_T("addr");
				md[2].key.len = 4;
				md[2].type = HAWK_VAL_MAP_DATA_STR;
				hawk_nwadtostr (&cfg.addr, addr, HAWK_COUNTOF(addr), HAWK_NWADTOSTR_ADDR);
				md[2].vptr = addr;

				md[3].key.ptr = HAWK_T("mask");
				md[3].key.len = 4;
				md[3].type = HAWK_VAL_MAP_DATA_STR;
				hawk_nwadtostr (&cfg.mask, mask, HAWK_COUNTOF(mask), HAWK_NWADTOSTR_ADDR);
				md[3].vptr = mask;

				md[4].key.ptr = HAWK_T("ethw");
				md[4].key.len = 4;
				md[4].type = HAWK_VAL_MAP_DATA_STR;
				hawk_strxfmt (ethw, HAWK_COUNTOF(ethw), HAWK_T("%02X:%02X:%02X:%02X:%02X:%02X"), 
					cfg.ethw[0], cfg.ethw[1], cfg.ethw[2], cfg.ethw[3], cfg.ethw[4], cfg.ethw[5]);
				md[4].vptr = ethw;

				if (cfg.flags & (HAWK_NWIFCFG_LINKUP | HAWK_NWIFCFG_LINKDOWN))
				{
					md[5].key.ptr = HAWK_T("link");
					md[5].key.len = 4;
					md[5].type = HAWK_VAL_MAP_DATA_STR;
					md[5].vptr = (cfg.flags & HAWK_NWIFCFG_LINKUP)? HAWK_T("up"): HAWK_T("down");
				}

				tmp = hawk_rtx_makemapvalwithdata (rtx, md);
				if (tmp)
				{
					int x;
					hawk_rtx_refupval (rtx, tmp);
					x = hawk_rtx_setrefval (rtx, (hawk_val_ref_t*)hawk_rtx_getarg(rtx, 2), tmp);
					hawk_rtx_refdownval (rtx, tmp);
					if (x <= -1) return -1;
					ret = 0;
				}
			}
		}
	}

	/* no error check for hawk_rtx_makeintval() since ret is 0 or -1 */
	hawk_rtx_setretval (rtx, hawk_rtx_makeintval (rtx, ret));
	return 0;
}
/* ------------------------------------------------------------ */

static int fnc_system (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_val_t* v, * a0;
	hawk_ooch_t* str;
	hawk_oow_t len;
	int n = 0;

	a0 = hawk_rtx_getarg (rtx, 0);
	str = hawk_rtx_getvaloocstr (rtx, a0, &len);
	if (str == HAWK_NULL) return -1;

	/* the target name contains a null character.
	 * make system return -1 */
	if (hawk_strxchr (str, len, HAWK_T('\0')))
	{
		n = -1;
		goto skip_system;
	}

#if defined(_WIN32)
	n = _tsystem (str);
#elif defined(HAWK_OOCH_IS_BCH)
	n = system (str);
#else

	{
		hawk_bch_t* mbs;
		mbs = hawk_wcstombsdupwithcmgr(str, HAWK_NULL, hawk_rtx_getmmgr(rtx), hawk_rtx_getcmgr(rtx));
		if (mbs == HAWK_NULL) 
		{
			n = -1;
			goto skip_system;
		}
		n = system (mbs);
		hawk_rtx_freemem (rtx, mbs);
	}

#endif

skip_system:
	hawk_rtx_freevaloocstr (rtx, a0, str);

	v = hawk_rtx_makeintval (rtx, (hawk_int_t)n);
	if (v == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, v);
	return 0;
}

/* ------------------------------------------------------------ */
static int fnc_chmod (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_val_t* v, * a0;
	hawk_ooch_t* str;
	hawk_oow_t len;
	int n = 0;
	hawk_int_t mode;

	a0 = hawk_rtx_getarg (rtx, 0);
	str = hawk_rtx_getvaloocstr (rtx, a0, &len);
	if (!str) return -1;

	/* the target name contains a null character.
	 * make system return -1 */
	if (hawk_strxchr(str, len, HAWK_T('\0')))
	{
		n = -1;
		goto skip_mkdir;
	}

	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 1), &mode) <= -1 || mode < 0) mode = DEFAULT_MODE;

#if defined(_WIN32)
	n = _tchmod(str, mode);
#elif defined(HAWK_OOCH_IS_BCH)
	n = chmod(str, mode);
#else

	{
		hawk_bch_t* mbs;
		mbs = hawk_wcstombsdupwithcmgr(str, HAWK_NULL, hawk_rtx_getmmgr(rtx), hawk_rtx_getcmgr(rtx));
		if (mbs == HAWK_NULL) 
		{
			n = -1;
			goto skip_mkdir;
		}
		n = chmod(mbs, mode);
		hawk_rtx_freemem (rtx, mbs);
	}

#endif

skip_mkdir:
	hawk_rtx_freevaloocstr (rtx, a0, str);

	v = hawk_rtx_makeintval (rtx, (hawk_int_t)n);
	if (v == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, v);
	return 0;
}

static int fnc_mkdir (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_val_t* v, * a0;
	hawk_ooch_t* str;
	hawk_oow_t len;
	int n = 0;
	hawk_int_t mode;

	a0 = hawk_rtx_getarg (rtx, 0);
	str = hawk_rtx_getvaloocstr (rtx, a0, &len);
	if (!str) return -1;

	/* the target name contains a null character.
	 * make system return -1 */
	if (hawk_strxchr(str, len, HAWK_T('\0')))
	{
		n = -1;
		goto skip_mkdir;
	}

	if (hawk_rtx_getnargs(rtx) >= 2 && (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 1), &mode) <= -1 || mode < 0)) mode = DEFAULT_MODE;

#if defined(_WIN32)
	n = _tmkdir(str);
#elif defined(HAWK_OOCH_IS_BCH)
	n = mkdir(str, mode);
#else

	{
		hawk_bch_t* mbs;
		mbs = hawk_wcstombsdupwithcmgr(str, HAWK_NULL, hawk_rtx_getmmgr(rtx), hawk_rtx_getcmgr(rtx));
		if (mbs == HAWK_NULL) 
		{
			n = -1;
			goto skip_mkdir;
		}
		n = mkdir(mbs, mode);
		hawk_rtx_freemem (rtx, mbs);
	}

#endif

skip_mkdir:
	hawk_rtx_freevaloocstr (rtx, a0, str);

	v = hawk_rtx_makeintval (rtx, (hawk_int_t)n);
	if (v == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, v);
	return 0;
}

static int fnc_unlink (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	hawk_val_t* v, * a0;
	hawk_ooch_t* str;
	hawk_oow_t len;
	int n = 0;

	a0 = hawk_rtx_getarg (rtx, 0);
	str = hawk_rtx_getvaloocstr(rtx, a0, &len);
	if (!str) return -1;

	/* the target name contains a null character.
	 * make system return -1 */
	if (hawk_strxchr(str, len, HAWK_T('\0')))
	{
		n = -1;
		goto skip_unlink;
	}

#if defined(_WIN32)
	n = _tunlink(str);
#elif defined(HAWK_OOCH_IS_BCH)
	n = unlink(str);
#else

	{
		hawk_bch_t* mbs;
		mbs = hawk_wcstombsdupwithcmgr(str, HAWK_NULL, hawk_rtx_getmmgr(rtx), hawk_rtx_getcmgr(rtx));
		if (mbs == HAWK_NULL) 
		{
			n = -1;
			goto skip_unlink;
		}
		n = unlink(mbs);
		hawk_rtx_freemem (rtx, mbs);
	}

#endif

skip_unlink:
	hawk_rtx_freevaloocstr (rtx, a0, str);

	v = hawk_rtx_makeintval (rtx, (hawk_int_t)n);
	if (v == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, v);
	return 0;
}

/* ------------------------------------------------------------ */
#if 0
static void open_remote_log_socket (hawk_rtx_t* rtx, mod_ctx_t* mctx)
{
#if defined(_WIN32)
	/* TODO: implement this */
#else
	int sck, flags;
	int domain = hawk_skadfamily(&mctx->log.skad);
	int type = SOCK_DGRAM;

	HAWK_ASSERT (mctx->log.sck <= -1);


#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
	type |= SOCK_NONBLOCK;
	type |= SOCK_CLOEXEC;
open_socket:
#endif
	sck = socket(domain, type, 0); 
	if (sck <= -1)
	{
	#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
		if (errno == EINVAL && (type & (SOCK_NONBLOCK | SOCK_CLOEXEC)))
		{
			type &= ~(SOCK_NONBLOCK | SOCK_CLOEXEC);
			goto open_socket;
		}
	#endif
		return;
	}
	else
	{
	#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
		if (type & (SOCK_NONBLOCK | SOCK_CLOEXEC)) goto done;
	#endif
	}

	flags = fcntl(sck, F_GETFD, 0);
	if (flags <= -1) return;
#if defined(FD_CLOEXEC)
	flags |= FD_CLOEXEC;
#endif
#if defined(O_NONBLOCK)
	flags |= O_NONBLOCK;
#endif
	if (fcntl(sck, F_SETFD, flags) <= -1) return;

done:
	mctx->log.sck = sck;

#endif
}

static int fnc_openlog (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	int rx = -1;
	hawk_int_t opt, fac;
	hawk_val_t* retv;
	hawk_ooch_t* ident = HAWK_NULL, * actual_ident;
	hawk_oow_t ident_len;
	hawk_bch_t* mbs_ident;
	mod_ctx_t* mctx = (mod_ctx_t*)fi->mod->ctx;
	hawk_nwad_t nwad;
	syslog_type_t log_type = SYSLOG_LOCAL;


	ident = hawk_rtx_getvaloocstr(rtx, hawk_rtx_getarg(rtx, 0), &ident_len);
	if (!ident) goto done;

	/* the target name contains a null character.
	 * make system return -1 */
	if (hawk_strxchr(ident, ident_len, HAWK_T('\0'))) goto done;

	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 1), &opt) <= -1) goto done;
	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 2), &fac) <= -1) goto done;

	if (hawk_strbeg(ident, HAWK_T("remote://")))
	{
		hawk_ooch_t* slash;
		/* "udp://remote-addr:remote-port/syslog-identifier" */

		log_type = SYSLOG_REMOTE;
		actual_ident = ident + 9;
		slash = hawk_strchr(actual_ident, HAWK_T('/'));
		if (!slash) goto done;
		if (hawk_strntonwad(actual_ident, slash - actual_ident, &nwad) <= -1) goto done;
		actual_ident = slash + 1;
	}
	else if (hawk_strbeg(ident, HAWK_T("local://")))
	{
		/* "local://syslog-identifier" */
		actual_ident = ident + 8;
	}
	else
	{
		actual_ident = ident;
	}

#if defined(HAWK_OOCH_IS_BCH)
	mbs_ident = hawk_becsdup(actual_ident, hawk_rtx_getmmgr(rtx));
#else
	mbs_ident = hawk_wcstombsdupwithcmgr(actual_ident, HAWK_NULL, hawk_rtx_getmmgr(rtx), hawk_rtx_getcmgr(rtx));
#endif
	if (!mbs_ident) goto done;

	if (mctx->log.ident) hawk_rtx_freemem (rtx, mctx->log.ident);
	mctx->log.ident = mbs_ident;

#if defined(ENABLE_SYSLOG)
	if (mctx->log.syslog_opened)
	{
		closelog ();
		mctx->log.syslog_opened = 0;
	}
#endif
	if (mctx->log.sck >= 0)
	{
	#if defined(_WIN32)
		/* TODO: impelement this */
	#else
		close (mctx->log.sck);
	#endif
		mctx->log.sck = -1;
	}

	mctx->log.type = log_type;
	mctx->log.opt = opt;
	mctx->log.fac = fac;
	if (mctx->log.type == SYSLOG_LOCAL)
	{
	#if defined(ENABLE_SYSLOG)
		openlog(mbs_ident, opt, fac);
		mctx->log.syslog_opened = 1;
	#endif
	}
	else if (mctx->log.type == SYSLOG_REMOTE)
	{
		hawk_nwadtoskad (&nwad, &mctx->log.skad);
		if ((opt & LOG_NDELAY) && mctx->log.sck <= -1) open_remote_log_socket (rtx, mctx);
	}

	rx = 0;

done:
	if (ident) hawk_rtx_freevaloocstr(rtx, hawk_rtx_getarg(rtx, 0), ident);

	retv = hawk_rtx_makeintval(rtx, rx);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_closelog (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	int rx = -1;
	hawk_val_t* retv;
	mod_ctx_t* mctx = (mod_ctx_t*)fi->mod->ctx;

	switch (mctx->log.type)
	{
		case SYSLOG_LOCAL:
		#if defined(ENABLE_SYSLOG)
			closelog ();
			/* closelog() might be called without openlog(). so there is no 
			 * check if syslog_opened is true.
			 * it is just used as an indicator to decide wheter closelog()
			 * should be called upon module finalization(fini). */
			mctx->log.syslog_opened = 0;
		#endif
			break;

		case SYSLOG_REMOTE:
			if (mctx->log.sck >= 0)
			{
			#if defined(_WIN32)
				/* TODO: impelement this */
			#else
				close (mctx->log.sck);
			#endif
				mctx->log.sck = -1;
			}

			if (mctx->log.dmsgbuf)
			{
				hawk_becs_close (mctx->log.dmsgbuf);
				mctx->log.dmsgbuf = HAWK_NULL;
			}

			break;
	}

	if (mctx->log.ident)
	{
		hawk_rtx_freemem (rtx, mctx->log.ident);
		mctx->log.ident = HAWK_NULL;
	}

	/* back to the local syslog in case writelog() is called
	 * without another openlog() after this closelog() */
	mctx->log.type = SYSLOG_LOCAL;

	rx = 0;

	retv = hawk_rtx_makeintval(rtx, rx);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}


static int fnc_writelog (hawk_rtx_t* rtx, const hawk_fnc_info_t* fi)
{
	int rx = -1;
	hawk_val_t* retv;
	hawk_int_t pri;
	hawk_ooch_t* msg = HAWK_NULL;
	hawk_oow_t msglen;
	mod_ctx_t* mctx = (mod_ctx_t*)fi->mod->ctx;

	if (hawk_rtx_valtoint(rtx, hawk_rtx_getarg(rtx, 0), &pri) <= -1) goto done;

	msg = hawk_rtx_getvaloocstr(rtx, hawk_rtx_getarg(rtx, 1), &msglen);
	if (!msg) goto done;

	if (hawk_strxchr(msg, msglen, HAWK_T('\0'))) goto done;

	if (mctx->log.type == SYSLOG_LOCAL)
	{
	#if defined(ENABLE_SYSLOG)
		#if defined(HAWK_OOCH_IS_BCH)
		syslog(pri, "%s", msg);
		#else
		{
			hawk_bch_t* mbs;
			mbs = hawk_wcstombsdupwithcmgr(msg, HAWK_NULL, hawk_rtx_getmmgr(rtx), hawk_rtx_getcmgr(rtx));
			if (!mbs) goto done;
			syslog(pri, "%s", mbs);
			hawk_rtx_freemem (rtx, mbs);
		}
		#endif
	#endif
	}
	else if (mctx->log.type == SYSLOG_REMOTE)
	{
	#if defined(_WIN32)
		/* TODO: implement this */
	#else
		hawk_ntime_t now;
		hawk_btime_t cnow;

		static const hawk_bch_t* __syslog_month_names[] =
		{
			HAWK_BT("Jan"),
			HAWK_BT("Feb"),
			HAWK_BT("Mar"),
			HAWK_BT("Apr"),
			HAWK_BT("May"),
			HAWK_BT("Jun"),
			HAWK_BT("Jul"),
			HAWK_BT("Aug"),
			HAWK_BT("Sep"),
			HAWK_BT("Oct"),
			HAWK_BT("Nov"),
			HAWK_BT("Dec"),
		};

		if (mctx->log.sck <= -1) open_remote_log_socket (rtx, mctx);

		if (mctx->log.sck >= 0)
		{
			if (!mctx->log.dmsgbuf) mctx->log.dmsgbuf = hawk_becs_open(hawk_rtx_getmmgr(rtx), 0, 0);
			if (!mctx->log.dmsgbuf) goto done;

			if (hawk_gettime(&now) || hawk_localtime(&now, &cnow) <= -1) goto done;

			if (hawk_becs_fmt (
				mctx->log.dmsgbuf, HAWK_BT("<%d>%s %02d %02d:%02d:%02d "), 
				(int)(mctx->log.fac | pri),
				__syslog_month_names[cnow.mon], cnow.mday, 
				cnow.hour, cnow.min, cnow.sec) == (hawk_oow_t)-1) goto done;

			if (mctx->log.ident || (mctx->log.opt & LOG_PID))
			{
				/* if the identifier is set or LOG_PID is set, the produced tag won't be empty.
				 * so appending ':' is kind of ok */

				if (hawk_becs_fcat(mctx->log.dmsgbuf, HAWK_BT("%hs"), (mctx->log.ident? mctx->log.ident: HAWK_BT(""))) == (hawk_oow_t)-1) goto done;

				if (mctx->log.opt & LOG_PID)
				{
					if (hawk_becs_fcat(mctx->log.dmsgbuf, HAWK_BT("[%d]"), (int)HAWK_GETPID()) == (hawk_oow_t)-1) goto done;
				}

				if (hawk_becs_fcat(mctx->log.dmsgbuf, HAWK_BT(": ")) == (hawk_oow_t)-1) goto done;
			}

		#if defined(HAWK_OOCH_IS_BCH)
			if (hawk_becs_fcat(mctx->log.dmsgbuf, HAWK_BT("%hs"), msg) == (hawk_oow_t)-1) goto done;
		#else
			if (hawk_becs_fcat(mctx->log.dmsgbuf, HAWK_BT("%ls"), msg) == (hawk_oow_t)-1) goto done;
		#endif

			/* don't care about output failure */
			sendto (mctx->log.sck, HAWK_MBS_PTR(mctx->log.dmsgbuf), HAWK_MBS_LEN(mctx->log.dmsgbuf),
			        0, (struct sockaddr*)&mctx->log.skad, hawk_skadsize(&mctx->log.skad));
		}
	#endif
	}

	rx = 0;

done:
	if (msg) hawk_rtx_freevaloocstr(rtx, hawk_rtx_getarg(rtx, 1), msg);

	retv = hawk_rtx_makeintval(rtx, rx);
	if (retv == HAWK_NULL) return -1;

	hawk_rtx_setretval (rtx, retv);
	return 0;
}

#endif
/* ------------------------------------------------------------ */

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const hawk_ooch_t* name;
	hawk_mod_sym_fnc_t info;
};

typedef struct inttab_t inttab_t;
struct inttab_t
{
	const hawk_ooch_t* name;
	hawk_mod_sym_int_t info;
};

static fnctab_t fnctab[] =
{
	/* keep this table sorted for binary search in query(). */

	{ HAWK_T("WCOREDUMP"),   { { 1, 1, HAWK_NULL     }, fnc_wcoredump,   0  } },
	{ HAWK_T("WEXITSTATUS"), { { 1, 1, HAWK_NULL     }, fnc_wexitstatus, 0  } },
	{ HAWK_T("WIFEXITED"),   { { 1, 1, HAWK_NULL     }, fnc_wifexited,   0  } },
	{ HAWK_T("WIFSIGNALED"), { { 1, 1, HAWK_NULL     }, fnc_wifsignaled, 0  } },
	{ HAWK_T("WTERMSIG"),    { { 1, 1, HAWK_NULL     }, fnc_wtermsig,    0  } },
	{ HAWK_T("chmod"),       { { 2, 2, HAWK_NULL     }, fnc_chmod,       0  } },
	{ HAWK_T("close"),       { { 1, 2, HAWK_NULL     }, fnc_close,       0  } },
	{ HAWK_T("closedir"),    { { 1, 1, HAWK_NULL     }, fnc_closedir,    0  } },
	{ HAWK_T("closelog"),    { { 0, 0, HAWK_NULL     }, fnc_closelog,    0  } },
	{ HAWK_T("dup"),         { { 1, 3, HAWK_NULL     }, fnc_dup,         0  } },
	{ HAWK_T("errmsg"),      { { 0, 0, HAWK_NULL     }, fnc_errmsg,      0  } },
	{ HAWK_T("fork"),        { { 0, 0, HAWK_NULL     }, fnc_fork,        0  } },
	{ HAWK_T("getegid"),     { { 0, 0, HAWK_NULL     }, fnc_getegid,     0  } },
	{ HAWK_T("getenv"),      { { 1, 1, HAWK_NULL     }, fnc_getenv,      0  } },
	{ HAWK_T("geteuid"),     { { 0, 0, HAWK_NULL     }, fnc_geteuid,     0  } },
	{ HAWK_T("getgid"),      { { 0, 0, HAWK_NULL     }, fnc_getgid,      0  } },
	{ HAWK_T("getnwifcfg"),  { { 3, 3, HAWK_T("vvr") }, fnc_getnwifcfg,  0  } },
	{ HAWK_T("getpgid"),     { { 0, 0, HAWK_NULL     }, fnc_getpgid,     0  } },
	{ HAWK_T("getpid"),      { { 0, 0, HAWK_NULL     }, fnc_getpid,      0  } },
	{ HAWK_T("getppid"),     { { 0, 0, HAWK_NULL     }, fnc_getppid,     0  } },
	{ HAWK_T("gettid"),      { { 0, 0, HAWK_NULL     }, fnc_gettid,      0  } },
	{ HAWK_T("gettime"),     { { 0, 0, HAWK_NULL     }, fnc_gettime,     0  } },
	{ HAWK_T("getuid"),      { { 0, 0, HAWK_NULL     }, fnc_getuid,      0  } },
	{ HAWK_T("kill"),        { { 2, 2, HAWK_NULL     }, fnc_kill,        0  } },
	{ HAWK_T("mkdir"),       { { 1, 2, HAWK_NULL     }, fnc_mkdir,       0  } },
	{ HAWK_T("mktime"),      { { 0, 1, HAWK_NULL     }, fnc_mktime,      0  } },
	{ HAWK_T("open"),        { { 2, 3, HAWK_NULL     }, fnc_open,        0  } },
	{ HAWK_T("opendir"),     { { 1, 2, HAWK_NULL     }, fnc_opendir,     0  } },
	{ HAWK_T("openfd"),      { { 1, 1, HAWK_NULL     }, fnc_openfd,      0  } },
	{ HAWK_T("openlog"),     { { 3, 3, HAWK_NULL     }, fnc_openlog,     0  } },
	{ HAWK_T("pipe"),        { { 2, 3, HAWK_T("rrv") }, fnc_pipe,        0  } },
	{ HAWK_T("read"),        { { 2, 3, HAWK_T("vrv") }, fnc_read,        0  } },
	{ HAWK_T("readdir"),     { { 2, 2, HAWK_T("vr")  }, fnc_readdir,     0  } },
	{ HAWK_T("settime"),     { { 1, 1, HAWK_NULL     }, fnc_settime,     0  } },
	{ HAWK_T("sleep"),       { { 1, 1, HAWK_NULL     }, fnc_sleep,       0  } },
	{ HAWK_T("strftime"),    { { 2, 3, HAWK_NULL     }, fnc_strftime,    0  } },
	{ HAWK_T("system"),      { { 1, 1, HAWK_NULL     }, fnc_system,      0  } },
	{ HAWK_T("systime"),     { { 0, 0, HAWK_NULL     }, fnc_gettime,     0  } }, /* alias to gettime() */
	{ HAWK_T("unlink"),      { { 1, 1, HAWK_NULL     }, fnc_unlink,      0  } },
	{ HAWK_T("wait"),        { { 1, 3, HAWK_T("vrv") }, fnc_wait,        0  } },
	{ HAWK_T("write"),       { { 2, 2, HAWK_NULL     }, fnc_write,       0  } },
	{ HAWK_T("writelog"),    { { 2, 2, HAWK_NULL     }, fnc_writelog,    0  } }
};

#if !defined(SIGHUP)
#	define SIGHUP 1
#endif
#if !defined(SIGINT)
#	define SIGINT 2
#endif
#if !defined(SIGQUIT)
#	define SIGQUIT 3
#endif
#if !defined(SIGABRT)
#	define SIGABRT 6
#endif
#if !defined(SIGKILL)
#	define SIGKILL 9
#endif
#if !defined(SIGSEGV)
#	define SIGSEGV 11
#endif
#if !defined(SIGALRM)
#	define SIGALRM 14
#endif
#if !defined(SIGTERM)
#	define SIGTERM 15
#endif

static inttab_t inttab[] =
{
	/* keep this table sorted for binary search in query(). */
	{ HAWK_T("C_KEEPFD"),           { CLOSE_KEEPFD } },

	{ HAWK_T("DIR_SORT"),           { HAWK_DIR_SORT } },

#if defined(ENABLE_SYSLOG)
	{ HAWK_T("LOG_FAC_AUTH"),       { LOG_AUTH } },
	{ HAWK_T("LOG_FAC_AUTHPRIV"),   { LOG_AUTHPRIV } },
	{ HAWK_T("LOG_FAC_CRON"),       { LOG_CRON } },
	{ HAWK_T("LOG_FAC_DAEMON"),     { LOG_DAEMON } },
	{ HAWK_T("LOG_FAC_FTP"),        { LOG_FTP } },
	{ HAWK_T("LOG_FAC_KERN"),       { LOG_KERN } },
	{ HAWK_T("LOG_FAC_LOCAL0"),     { LOG_LOCAL0 } },
	{ HAWK_T("LOG_FAC_LOCAL1"),     { LOG_LOCAL1 } },
	{ HAWK_T("LOG_FAC_LOCAL2"),     { LOG_LOCAL2 } },
	{ HAWK_T("LOG_FAC_LOCAL3"),     { LOG_LOCAL3 } },
	{ HAWK_T("LOG_FAC_LOCAL4"),     { LOG_LOCAL4 } },
	{ HAWK_T("LOG_FAC_LOCAL5"),     { LOG_LOCAL5 } },
	{ HAWK_T("LOG_FAC_LOCAL6"),     { LOG_LOCAL6 } },
	{ HAWK_T("LOG_FAC_LOCAL7"),     { LOG_LOCAL7 } },
	{ HAWK_T("LOG_FAC_LPR"),        { LOG_LPR } },
	{ HAWK_T("LOG_FAC_MAIL"),       { LOG_MAIL } },
	{ HAWK_T("LOG_FAC_NEWS"),       { LOG_NEWS } },
	{ HAWK_T("LOG_FAC_SYSLOG"),     { LOG_SYSLOG } },
	{ HAWK_T("LOG_FAC_USER"),       { LOG_USER } },
	{ HAWK_T("LOG_FAC_UUCP"),       { LOG_UUCP } },

	{ HAWK_T("LOG_OPT_CONS"),       { LOG_CONS } },
	{ HAWK_T("LOG_OPT_NDELAY"),     { LOG_NDELAY } },
	{ HAWK_T("LOG_OPT_NOWAIT"),     { LOG_NOWAIT } },
	{ HAWK_T("LOG_OPT_PID"),        { LOG_PID } },

	{ HAWK_T("LOG_PRI_ALERT"),      { LOG_ALERT } },
	{ HAWK_T("LOG_PRI_CRIT"),       { LOG_CRIT } },
	{ HAWK_T("LOG_PRI_DEBUG"),      { LOG_DEBUG } },
	{ HAWK_T("LOG_PRI_EMERG"),      { LOG_EMERG } },
	{ HAWK_T("LOG_PRI_ERR"),        { LOG_ERR } },
	{ HAWK_T("LOG_PRI_INFO"),       { LOG_INFO } },
	{ HAWK_T("LOG_PRI_NOTICE"),     { LOG_NOTICE } },
	{ HAWK_T("LOG_PRI_WARNING"),    { LOG_WARNING } },
#endif

	{ HAWK_T("NWIFCFG_IN4"), { HAWK_NWIFCFG_IN4 } },
	{ HAWK_T("NWIFCFG_IN6"), { HAWK_NWIFCFG_IN6 } },

#if defined(O_APPEND)
	{ HAWK_T("O_APPEND"),    { O_APPEND } },
#endif
#if defined(O_ASYNC)
	{ HAWK_T("O_ASYNC"),     { O_ASYNC } },
#endif
#if defined(O_CLOEXEC)
	{ HAWK_T("O_CLOEXEC"),   { O_CLOEXEC } },
#endif
#if defined(O_CREAT)
	{ HAWK_T("O_CREAT"),     { O_CREAT } },
#endif
#if defined(O_DIRECTORY)
	{ HAWK_T("O_DIRECTORY"), { O_DIRECTORY } },
#endif
#if defined(O_DSYNC)
	{ HAWK_T("O_DSYNC"),     { O_DSYNC } },
#endif
#if defined(O_EXCL)
	{ HAWK_T("O_EXCL"),      { O_EXCL } },
#endif
#if defined(O_NOATIME)
	{ HAWK_T("O_NOATIME"),   { O_NOATIME} },
#endif
#if defined(O_NOCTTY)
	{ HAWK_T("O_NOCTTY"),    { O_NOCTTY} },
#endif
#if defined(O_NOFOLLOW)
	{ HAWK_T("O_NOFOLLOW"),  { O_NOFOLLOW } },
#endif
#if defined(O_NONBLOCK)
	{ HAWK_T("O_NONBLOCK"),  { O_NONBLOCK } },
#endif
#if defined(O_RDONLY)
	{ HAWK_T("O_RDONLY"),    { O_RDONLY } },
#endif
#if defined(O_RDWR)
	{ HAWK_T("O_RDWR"),      { O_RDWR } },
#endif
#if defined(O_SYNC)
	{ HAWK_T("O_SYNC"),      { O_SYNC } },
#endif
#if defined(O_TRUNC)
	{ HAWK_T("O_TRUNC"),     { O_TRUNC } },
#endif
#if defined(O_WRONLY)
	{ HAWK_T("O_WRONLY"),    { O_WRONLY } },
#endif

	{ HAWK_T("RC_EAGAIN"),  { RC_EAGAIN } },
	{ HAWK_T("RC_EBADF"),   { RC_EBADF } },
	{ HAWK_T("RC_ECHILD"),  { RC_ECHILD } },
	{ HAWK_T("RC_EEXIST"),  { RC_EEXIST } },
	{ HAWK_T("RC_EINTR"),   { RC_EINTR } },
	{ HAWK_T("RC_EINVAL"),  { RC_EINVAL } },
	{ HAWK_T("RC_ENOENT"),  { RC_ENOENT } },
	{ HAWK_T("RC_ENOIMPL"), { RC_ENOIMPL } },
	{ HAWK_T("RC_ENOMEM"),  { RC_ENOMEM } },
	{ HAWK_T("RC_ENOTDIR"), { RC_ENOTDIR } },
	{ HAWK_T("RC_ENOSYS"),  { RC_ENOSYS } },
	{ HAWK_T("RC_EPERM"),   { RC_EPERM } },
	{ HAWK_T("RC_ERROR"),   { RC_ERROR } },

	{ HAWK_T("SIGABRT"), { SIGABRT } },
	{ HAWK_T("SIGALRM"), { SIGALRM } },
	{ HAWK_T("SIGHUP"),  { SIGHUP } },
	{ HAWK_T("SIGINT"),  { SIGINT } },
	{ HAWK_T("SIGKILL"), { SIGKILL } },
	{ HAWK_T("SIGQUIT"), { SIGQUIT } },
	{ HAWK_T("SIGSEGV"), { SIGSEGV } },
	{ HAWK_T("SIGTERM"), { SIGTERM } },

	{ HAWK_T("STRFTIME_UTC"), { STRFTIME_UTC } },

	{ HAWK_T("WNOHANG"), { WNOHANG } }
};

static int query (hawk_mod_t* mod, hawk_t* awk, const hawk_ooch_t* name, hawk_mod_sym_t* sym)
{
	hawk_oocs_t ea;
	int left, right, mid, n;

	left = 0; right = HAWK_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = left + (right - left) / 2;

		n = hawk_comp_oocstr(fnctab[mid].name, name, 0);
		if (n > 0) right = mid - 1; 
		else if (n < 0) left = mid + 1;
		else
		{
			sym->type = HAWK_MOD_FNC;
			sym->u.fnc = fnctab[mid].info;
			return 0;
		}
	}

	left = 0; right = HAWK_COUNTOF(inttab) - 1;
	while (left <= right)
	{
		mid = left + (right - left) / 2;

		n = hawk_comp_oocstr(inttab[mid].name, name, 0);
		if (n > 0) right = mid - 1; 
		else if (n < 0) left = mid + 1;
		else
		{
			sym->type = HAWK_MOD_INT;
			sym->u.in = inttab[mid].info;
			return 0;
		}
	}

	ea.ptr = (hawk_ooch_t*)name;
	ea.len = hawk_count_oocstr(name);
	hawk_seterror (awk, HAWK_ENOENT, &ea, HAWK_NULL);
	return -1;
}

/* TODO: proper resource management */

static int init (hawk_mod_t* mod, hawk_rtx_t* rtx)
{
	mod_ctx_t* mctx = (mod_ctx_t*)mod->ctx;
	rtx_data_t data;

#if 0
	mctx->log.type = SYSLOG_LOCAL;
	mctx->log.syslog_opened = 0;
	mctx->log.sck = -1;
#endif

	HAWK_MEMSET (&data, 0, HAWK_SIZEOF(data));
	if (hawk_rbt_insert(mctx->rtxtab, &rtx, HAWK_SIZEOF(rtx), &data, HAWK_SIZEOF(data)) == HAWK_NULL) return -1;

	return 0;
}

static void fini (hawk_mod_t* mod, hawk_rtx_t* rtx)
{
	/* TODO: 
	for (each pid for rtx) kill (pid, SIGKILL);
	for (each pid for rtx) waitpid (pid, HAWK_NULL, 0);
	*/

	mod_ctx_t* mctx = (mod_ctx_t*)mod->ctx;
	hawk_rbt_pair_t* pair;

	/* garbage clean-up */
	pair = hawk_rbt_search(mctx->rtxtab, &rtx, HAWK_SIZEOF(rtx));
	if (pair)
	{
		rtx_data_t* data;
		sys_node_t* sys_node, * sys_next;

		data = (rtx_data_t*)HAWK_RBT_VPTR(pair);

		sys_node = data->sys_list.head;
		while (sys_node)
		{
			sys_next = sys_node->next;
			free_sys_node (rtx, &data->sys_list, sys_node);
			sys_node = sys_next;
		}

		if (data->sys_list.ctx.readbuf)
		{
			hawk_rtx_freemem (rtx, data->sys_list.ctx.readbuf);
			data->sys_list.ctx.readbuf = HAWK_NULL;
			data->sys_list.ctx.readbuf_capa = 0;
		}
		hawk_rbt_delete (mctx->rtxtab, &rtx, HAWK_SIZEOF(rtx));
	}


#if 0
#if defined(ENABLE_SYSLOG)
	if (mctx->log.syslog_opened) 
	{
		/* closelog() only if openlog() has been called explicitly.
		 * if you call writelog() functions without openlog() and
		 * end yoru program without closelog(), the program may leak
		 * some resources created by the writelog() function. (e.g.
		 * socket to /dev/log) */
		closelog ();
		mctx->log.syslog_opened = 0;
	}
#endif
	
	if (mctx->log.sck >= 0)
	{
	#if defined(_WIN32)
		/* TODO: implement this */
	#else
		close (mctx->log.sck);
	#endif
		mctx->log.sck = -1;
	}

	if (mctx->log.dmsgbuf)
	{
		hawk_becs_close (mctx->log.dmsgbuf);
		mctx->log.dmsgbuf = HAWK_NULL;
	}

	if (mctx->log.ident) 
	{
		hawk_rtx_freemem (rtx, mctx->log.ident);
		mctx->log.ident = HAWK_NULL;
	}
#endif
}

static void unload (hawk_mod_t* mod, hawk_t* awk)
{
	mod_ctx_t* mctx = (mod_ctx_t*)mod->ctx;

	HAWK_ASSERT (awk, HAWK_RBT_SIZE(mctx->rtxtab) == 0);
	hawk_rbt_close (mctx->rtxtab);

	hawk_freemem (awk, mctx);
}

int hawk_mod_sys (hawk_mod_t* mod, hawk_t* awk)
{
	hawk_rbt_t* rbt;

	mod->query = query;
	mod->unload = unload;

	mod->init = init;
	mod->fini = fini;

	mod->ctx = hawk_callocmem(awk, HAWK_SIZEOF(mod_ctx_t));
	if (!mod->ctx) return -1;

	rbt = hawk_rbt_open(hawk_getgem(awk), 0, 1, 1);
	if (rbt == HAWK_NULL) 
	{
		hawk_freemem (awk, mod->ctx);
		return -1;
	}
	hawk_rbt_setstyle (rbt, hawk_get_rbt_style(HAWK_RBT_STYLE_INLINE_COPIERS));

	((mod_ctx_t*)mod->ctx)->rtxtab = rbt;
	return 0;
}
