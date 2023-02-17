/*
 * File: env/freebsd/udi_osdep.c
 *
 * FreeBSD specific general environment code.
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

#include <sys/conf.h>
#include <sys/syslog.h>
#include <sys/kthread.h>

/*
 * Globals.
 */
_OSDEP_EVENT_T _udi_alloc_event;

/*
 * Privates.
 */
static _OSDEP_EVENT_T _udi_ev;
static udi_boolean_t _udi_kill_daemon;
static struct proc *_udi_alloc_daemon_thread;

void *
_udi_malloc(udi_size_t size, int flags, int sleep_ok)
{
        void *n;

/* if (!sleep_ok) return 0; */
	n = malloc(size, M_DEVBUF, 
		sleep_ok == UDI_NOWAIT ? M_NOWAIT : M_WAITOK);
        if ((!(flags & UDI_MEM_NOZERO)) && n)
                bzero(n, size);
        return n;
}

udi_ubit32_t 
_udi_curr_time_in_ticks(void)
{
	struct timeval tv;
	getmicrouptime(&tv);
	return tvtohz(&tv);
}

void
__udi_assert(const char *str,
             const char *file,
             int line)
{
        uprintf("Assertion ( %s ) Failed: File = %s, Line %d",
                str, file, line);
}

udi_ubit32_t
udi_strtou32 (const char *s,
		char **endptr,
		int base)

{
	return((udi_ubit32_t)strtoul(s, endptr, base));
}

void _udi_readfile_getdata(const char *filename, udi_size_t offset,
                           char *buf, udi_size_t buf_len,
                           udi_size_t * act_len)
{
}

char *
_udi_msgfile_search(const char *filename, udi_ubit32_t msgnum,
                          const char *locale)
{
	return NULL;
}

/*
 * Function to fill in udi_limit_t struct for region creation.
 */
void _udi_get_limits_t(udi_limits_t * limits)
{
	/* These could ultimately be tied into the KMA */
	limits->max_legal_alloc = 128 * 1024;
	limits->max_safe_alloc = UDI_MIN_ALLOC_LIMIT;
	limits->max_trace_log_formatted_len = _OSDEP_TRLOG_LIMIT;
	limits->max_instance_attr_len = UDI_MIN_INSTANCE_ATTR_LIMIT;
	limits->min_curtime_res = 1000000000 / hz;
	limits->min_timer_res = limits->min_curtime_res;
}

udi_boolean_t 
_udi_osdep_log_data(char *fmtbuf,
		    udi_size_t size,
		    struct _udi_cb *cb)
{
	int sev = LOG_DEBUG;
	switch (cb->cb_callargs.logm.logm_severity) {
		case UDI_LOG_DISASTER:
			sev = LOG_EMERG;
			break;
		case UDI_LOG_ERROR:
			sev = LOG_ERR;
			break;
		case UDI_LOG_WARNING:
			sev = LOG_WARNING;
			break;
		case UDI_LOG_INFORMATION:
			sev = LOG_INFO;
			break;
	}
	log(sev, "%s", fmtbuf);
	return UDI_OK;
}

static dev_t udidev;

static struct cdevsw udi_cdevsw = {
        noopen,
        noclose,
        noread,
        nowrite,
        noioctl,
        nopoll,
        nommap,
        nostrategy,
        "example",
        34,                     /* /usr/src/sys/conf/majors */
        nodump,
        nopsize,
        D_TTY,
        -1 /* block major */
};


static void
udi_unload_complete(void)
{
        _OSDEP_EVENT_SIGNAL(&_udi_ev);
}

int maxwork = 0;
static void 
_udi_alloc_daemon(void *arg)
{
	while (!_udi_kill_daemon) {
		if (!_udi_alloc_daemon_work()) {
			_OSDEP_EVENT_WAIT(&_udi_alloc_event);
		} else {
			_OSDEP_EVENT_CLEAR(&_udi_alloc_event);
		}
	}

printf("Kill daemon req\n");
	wakeup(&_udi_kill_daemon);
printf("Calling _exit\n");
	kthread_exit(0);
		
}


static int
udi_load(struct module *mod, int action, void *arg)
{
	int err = 0 ;
	switch (action) {
	case MOD_LOAD:
		udidev = make_dev(&udi_cdevsw,
				0,
				UID_ROOT,
				GID_WHEEL,
				0x600,
				"UDI core environment");

		_OSDEP_EVENT_INIT (&_udi_ev);
		_OSDEP_EVENT_INIT (&_udi_alloc_event);

		_udi_alloc_init();
		_udi_MA_init();
		uprintf("UDI loaded.  %x\n", udidev);

		if (kthread_create(_udi_alloc_daemon, NULL, 
				   &_udi_alloc_daemon_thread, "udi_memd")) {
			printf("kthread_create failed!\n");
		}

		
		break;
	case MOD_UNLOAD:
		 _udi_MA_deinit (udi_unload_complete);
		_OSDEP_EVENT_WAIT (&_udi_ev);

		_udi_kill_daemon = TRUE;
		/* 
		 * FIXME: For some reason, nuking the alloc daemon 
		 * results in badness. 
		 */
		_OSDEP_EVENT_SIGNAL(&_udi_alloc_event);
		tsleep(&_udi_kill_daemon, PZERO, "udiallocdeath", 0);

#if 0
		_OSDEP_EVENT_WAIT (&_udi_ev);
#endif

		_OSDEP_EVENT_DEINIT(&_udi_ev);
		_OSDEP_EVENT_DEINIT(&_udi_alloc_event);

		destroy_dev(udidev);

		uprintf("UDI unloaded.  %x\n", udidev);
		break;
	default:
		err = EINVAL;
		break;
	}
	
	return err;
}

DEV_MODULE(udi_core_environment, udi_load, NULL);
