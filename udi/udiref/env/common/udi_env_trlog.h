
/*
 * File: env/common/udi_env_trlog.h
 *
 * UDI Common Environment tracing and logging definitions; extends udi_env.h.
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

#ifndef _UDI_ENV_TRLOG_H
#define _UDI_ENV_TRLOG_H


/* Use some holes in the udi_trevent_t definitions for environment specific
 * things.  If the udi_trevent_t changes these will need to be moved...
 * Also synchronize the udi_trevent_name array in udi_trlog.c with these.
 */

#define _UDI_TREVENT_ENVERR  (1U<<3)
#define _UDI_TREVENT_ENVINFO (1U<<4)


/*
 * Set defaults for TRLOG handling if not already provided in osdep.h.
 *
 *  There is quite a bit of customization allowed here; the defaults
 *  provided will support at least moderate levels of tracing to
 *  stdout via printf operations and are a good place to start if
 *  possible.
 */

#ifndef _OSDEP_TRLOG_LIMIT		/* udi_limits_t.max_trace_log_formatted_len */
#define _OSDEP_TRLOG_LIMIT  UDI_MIN_TRACE_LOG_LIMIT
#endif

#ifndef _OSDEP_TRLOG_SIZE		/* Number of entries in the trlog ring buffer */
#define _OSDEP_TRLOG_SIZE 20		/* should be udi_index_t addressible */
#endif

#ifndef _OSDEP_TRLOG_OLDEST		/* Set to 1 to keep oldest entries if full */
#define _OSDEP_TRLOG_OLDEST 0		/* no, keep newest */
#endif


#ifndef _OSDEP_TRLOG_INIT		/* Called at env init for trlog specific init */
#define _OSDEP_TRLOG_INIT _udi_trlog_initial = 0xffffffff; { int ii;	 \
                          for (ii=0; ii < 256; ii++)			 \
                              _udi_trlog_ml_initial[ii] = 0xffffffff; }
#endif

#ifndef _OSDEP_TRLOG_DONE
#define _OSDEP_TRLOG_DONE ;
#endif

#ifndef _OSDEP_TRLOG_NEW		/* Called when a new entry added to the ring */
#define _OSDEP_TRLOG_NEW  _udi_trlog_daemon_work();
#endif

#ifndef _OSDEP_TRACE_DATA		/* Called to output formatted trace data */
#define _OSDEP_TRACE_DATA(S,L) _OSDEP_PRINTF("%s\n",(S))
#endif

#ifndef _OSDEP_LOG_DATA			/* Called to output formatted log data;
					 * expected to return 0 if the log
					 * callback should be done; non-zero means
					 * that this operation will handle the
					 * callback event. */
#define _OSDEP_LOG_DATA(S,L,CB) (_OSDEP_PRINTF("%s\n",(S)), 0)
#endif

#ifndef _OSDEP_TIMESTAMP_STR		/* Convert timestamp to string for trlog */
#define _OSDEP_TIMESTAMP_STR(B,L,T) udi_snprintf((B), (L), "%d", (T));
#endif


#ifndef _OSDEP_NUM_LOGCB		/* # of MA CB's for environment internal logging */
#define _OSDEP_NUM_LOGCB  10
#endif


/*
 * Internal structures to represent trace or log events to be handled
 */


typedef struct {			/* Common information for any trace/log event */
	udi_timestamp_t trlog_timestamp;
	udi_trevent_t trlog_event;
	udi_index_t trlog_meta_idx;
	char trlog_data[_OSDEP_TRLOG_LIMIT];
} _udi_trlog_info_t;

typedef struct {			/* Structure of ring buffer entries for trace events */
	_udi_trlog_info_t trlog_info;
	_udi_region_t *trlog_region;
} _udi_trlog_t;

typedef struct _udi_log_marshal {	/* udi_log_write cb marshalling */
	_udi_trlog_info_t trlog_info;
	udi_ubit8_t logm_severity;
	udi_status_t logm_status;
} _udi_log_marshal_t;

/* Single global ring buffer for trace events; added to by trace and
   event log write operations; serviced by the trlog thread.  Set
   policies via: _OSDEP_TRLOG_SIZE and _OSDEP_TRLOG_OLDEST (default=0
   which is keep newest) */
extern _udi_trlog_t _udi_trlog_events[_OSDEP_TRLOG_SIZE];
extern udi_index_t _udi_trlog_producer_idx,	/* next ring slot to fill */
  _udi_trlog_consumer_idx;		/* next ring slot to handle */
extern udi_queue_t _udi_trlog_q;	/* Q head for log_write cb's */
extern _OSDEP_SIMPLE_MUTEX_T _udi_trlog_mutex;	/* 4 trlog_xxx_idx's & trlog_Q */
extern udi_boolean_t _udi_trlog_daemon_work(void);	/* handles trlog events */

extern udi_trevent_t _udi_trlog_initial;	/* dflt new region mask */
extern udi_trevent_t _udi_trlog_ml_initial[256];	/* dflt new region ml masks */

extern udi_boolean_t _udi_osdep_log_data(char *,
					 udi_size_t,
					 struct _udi_cb *);

void _udi_trlog_init(void);
void _udi_trlog_deinit(void);

#endif /* _UDI_ENV_TRLOG_H */
