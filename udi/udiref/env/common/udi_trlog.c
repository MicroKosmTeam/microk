
/*
 * File: env/common/udi_trlog.c
 *
 * UDI Tracing and Logging Routines
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
#include <udi_mgmt_MA.h>


#define UDI_QUEUE_MOVE(FromHead,ToHead) {	\
        if (!UDI_QUEUE_EMPTY(FromHead)) {	\
            *(ToHead) = *(FromHead);		\
	    (ToHead)->next->prev = (ToHead);	\
	    (ToHead)->prev->next = (ToHead);	\
            UDI_QUEUE_INIT(FromHead);		\
        } else {				\
            UDI_QUEUE_INIT(ToHead);		\
        }					\
    }

#define UDI_TREVENT_META_MASK 0x0000F9C0


STATIC char udi_trevent_name[] =
    /*
     * Must correspond to bit0 thru bit31 trace events in udi_trace_log.h 
     */
	"%<0=Local proc entry,1=Local proc exit,2=External error," "3=ENV Error,"	/* env extension; not in UDI spec */
	"4=ENV Info,"			/* env extension; not in UDI spec */
	"6=I/O scheduled,7=I/O completed,8=State Change," "9=Env Error,"	/* env extension; not in UDI spec */
	"11=Meta1,12=Meta2,13=Meta3,14=Meta4,15=Meta5,"
	"16=Drvr1,17=Drvr2,18=Drvr3,19=Drvr4,20=Drvr5,21=Drvr6,"
	"22=Drvr7,23=Drvr8,24=Drvr9,25=Drvr10,26=Drvr11,27=Drvr12,"
	"28=Drvr13,29=Drvr14,30=Drvr15,31=Drvr16>";

STATIC char udi_log_severities[][16] = { "INVALID_LOG", "DISASTER", "ERROR",
	"Warning", "LOG"
};

    /*
     * Must correspond to UDI_LOG_xxx severity levels 
     */


#define UDI_STATUS_NAMES "%<0-14=Status:0=OK:1=Not Supported:"		     \
                         "2=Not Understood:3=Invalid State:"		     \
                         "4=Mistaken Identity:5=Aborted:6=Timeout:"	     \
                         "7=Busy:8=Resource Unavaliable:9=Hardware Problem:" \
                         "10=Device Not Responding:11=Data Underrun:"	     \
                         "12=Data Overrun:13=Data Error:"		     \
			 "14=Parent Driver Error:"			     \
                         "15=Bind Failed:16=Cannot Bind Exclusively:"	     \
                         "17=Too Many Parents:18=Bad Parent Type>"


/*
 * Global trace/log ring buffer for events (see udi_env.h)
 */

_udi_trlog_t _udi_trlog_events[_OSDEP_TRLOG_SIZE];
udi_index_t _udi_trlog_producer_idx;
udi_index_t _udi_trlog_consumer_idx;
udi_queue_t _udi_trlog_q;

udi_trevent_t _udi_trlog_initial;
udi_trevent_t _udi_trlog_ml_initial[256];

_OSDEP_SIMPLE_MUTEX_T _udi_trlog_mutex;

STATIC int _udi_trlog_correlate;	/* Maintains the current correlate information */

STATIC udi_queue_t _udi_MA_logcbQ;	/* for env-internal logging */
STATIC udi_boolean_t _udi_MA_logcb_alloc_done;


STATIC void _udi_log_vwrite(udi_log_write_call_t *callback,
			    udi_cb_t *gcb,
			    udi_trevent_t trace_event,
			    udi_ubit8_t severity,
			    udi_index_t meta_idx,
			    udi_status_t original_status,
			    udi_ubit32_t msgnum,
			    va_list arg);

STATIC udi_boolean_t udi_trlog_running = FALSE;

/*
 * UDI Trace/Log Public Interface
 */


/* udi_trace_write
 *
 * This is the UDI environment service used for tracing events.  See the
 * UDI Core Specification Tracing and Logging Chapter for more information.
 */

void
udi_trace_write(udi_init_context_t *init_context,
		udi_trevent_t trace_event,
		udi_index_t meta_idx,
		udi_ubit32_t msgnum,
		...)
{
	udi_index_t new_idx;
	_udi_trlog_t *ptrlog;
	_udi_region_t *region, **prgnp;
	char *trcmsg, *str;
	va_list arg;

	_OSDEP_ASSERT(udi_trlog_running == TRUE);
	/*
	 * Validate input arguments and that we wish to trace this event. 
	 */

	UDIENV_CHECK(cannot_log_trevent_log_via_udi_trace_write,
		     trace_event != UDI_TREVENT_LOG);

	if (init_context == NULL)
		return;			/* too early in init to trace... */

	prgnp = (void *)init_context;
	region = *(--prgnp);		/* Specially setup by _udi_region_create */
	/*
	 * If the region is being torn down, don't log messages 
	 */
	if (region->reg_self_destruct) {
		return;
	}

	if (trace_event & UDI_TREVENT_META_MASK) {
		if ((region->reg_ml_tracemask[meta_idx] & trace_event) == 0) {
			return;
		}
	} else {
		if ((region->reg_tracemask & trace_event) == 0) {
			return;
		}
	}

	trcmsg = _udi_osdep_sprops_ifind_message(region->reg_driver,
						 _udi_current_locale(NULL),
						 msgnum);
	if (trcmsg)
		str = _udi_attr_string_decode(region->reg_driver,
					      _udi_current_locale(NULL),
					      trcmsg);
	else
		str = NULL;

	/*
	 * Note; udi_attr_string_decode allocates memory for the
	 * decoded string and may sleep waiting for available
	 * memory... call this before acquiring the _udi_trlog_mutex below.
	 */

	/*
	 * First determine where in the trlog ring to place this new event
	 * (if there's room). 
	 */

	_OSDEP_SIMPLE_MUTEX_LOCK(&_udi_trlog_mutex);

	new_idx = _udi_trlog_producer_idx;
	ptrlog = &_udi_trlog_events[new_idx];
	if (++new_idx == _OSDEP_TRLOG_SIZE)
		new_idx = 0;
	if (new_idx == _udi_trlog_consumer_idx) {
		if (_OSDEP_TRLOG_OLDEST == 0) {
			if (++_udi_trlog_consumer_idx == _OSDEP_TRLOG_SIZE)
				_udi_trlog_consumer_idx = 0;
		} else {
			_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_trlog_mutex);
			return;
		}
	}
	ptrlog->trlog_info.trlog_event = 0;	/* mark as in-progress */
	_udi_trlog_producer_idx = new_idx;
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_trlog_mutex);


	/*
	 * Now fill in this trlog event entry 
	 */
	ptrlog->trlog_info.trlog_timestamp = udi_time_current();
	ptrlog->trlog_info.trlog_meta_idx = meta_idx;
	ptrlog->trlog_region = region;

	/*
	 * A more elegant implementation could improve performance/memory-usage
	 * at this point by simply saving the msgnum and using a (pre-parsed)
	 * version of the message text to save off the varargs.  However, we'll
	 * just keep things simple.
	 */
	va_start(arg, msgnum);
	if (trcmsg) {
		udi_vsnprintf(ptrlog->trlog_info.trlog_data,
			      sizeof (ptrlog->trlog_info.trlog_data),
			      str, arg);
		_OSDEP_MEM_FREE(str);
		_OSDEP_MEM_FREE(trcmsg);
	} else {
		udi_snprintf(ptrlog->trlog_info.trlog_data,
			     sizeof (ptrlog->trlog_info.trlog_data),
			     "Trace MSG #%d", msgnum);
	}
	va_end(arg);
	ptrlog->trlog_info.trlog_data[sizeof (ptrlog->trlog_info.trlog_data) -
				      1] = 0;

	/*
	 * setting the trlog_event should be the last thing because it
	 * also signals to the logging daemon that this event may now
	 * be processed 
	 */

	ptrlog->trlog_info.trlog_event = trace_event;

	/*
	 * Now schedule OS handling of this new trlog event 
	 * (e.g. wakeup daemon) 
	 */
	_OSDEP_TRLOG_NEW;
}


/* udi_log_write
 *
 * This is the UDI environment service used for logging events.  See the
 * UDI Core Specification Tracing and Logging Chapter for more information.
 */

void
udi_log_write(udi_log_write_call_t *callback,
	      udi_cb_t *gcb,
	      udi_trevent_t trace_event,
	      udi_ubit8_t severity,
	      udi_index_t meta_idx,
	      udi_status_t original_status,
	      udi_ubit32_t msgnum,
	      ...)
{
	va_list arg;

	va_start(arg, msgnum);
	_OSDEP_ASSERT(UDI_LOG_DISASTER <= severity &&
		      severity <= UDI_LOG_INFORMATION);
	_udi_log_vwrite(callback, gcb, trace_event, severity, meta_idx,
			original_status, msgnum, arg);
	va_end(arg);
}


/*
 * Internal support operations
 */

STATIC void
_udi_log_vwrite(udi_log_write_call_t *callback,
		udi_cb_t *gcb,
		udi_trevent_t trace_event,
		udi_ubit8_t severity,
		udi_index_t meta_idx,
		udi_status_t original_status,
		udi_ubit32_t msgnum,
		va_list arg)
{

	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_log_marshal_t *plogm = &cb->cb_callargs.logm;
	_udi_driver_t *pdriver;
	char *logmsg;

	_OSDEP_ASSERT(udi_trlog_running == TRUE);

	UDIENV_CHECK(cb_has_valid_channel_for_log_write,
		     _UDI_CB_TO_CHAN(cb) != NULL);
	/*
	 * ... even gcb_alloc should assign a channel to identify the
	 * region for the cb 
	 */
	pdriver = _UDI_GCB_TO_CHAN(gcb)->chan_region->reg_driver;

	cb->cb_func = (_udi_callback_func_t *)callback;
	cb->cb_flags = _UDI_CB_CALLBACK;
	cb->op_idx = _UDI_CT_LOG_WRITE;	/* for callback queueing/delivery */

	plogm->trlog_info.trlog_timestamp = udi_time_current();
	plogm->trlog_info.trlog_event = trace_event;
	plogm->trlog_info.trlog_meta_idx = meta_idx;
	plogm->logm_severity = severity;
	plogm->logm_status = original_status;

	logmsg = _udi_osdep_sprops_ifind_message(pdriver,
						 _udi_current_locale(NULL),
						 msgnum);
	if (logmsg) {
		char *str = _udi_attr_string_decode(pdriver,
						    _udi_current_locale(NULL),

						    logmsg);

		udi_vsnprintf(plogm->trlog_info.trlog_data,
			      sizeof (plogm->trlog_info.trlog_data), str, arg);
		_OSDEP_MEM_FREE(str);
		_OSDEP_MEM_FREE(logmsg);
	} else {
		udi_snprintf(plogm->trlog_info.trlog_data,
			     sizeof (plogm->trlog_info.trlog_data),
			     "Log MSG #%d", msgnum);
	}

	plogm->trlog_info.trlog_data[sizeof (plogm->trlog_info.trlog_data) -
				     1] = 0;

	_OSDEP_SIMPLE_MUTEX_LOCK(&_udi_trlog_mutex);
	UDI_ENQUEUE_TAIL(&_udi_trlog_q, &cb->cb_qlink);
	_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_trlog_mutex);
	_OSDEP_TRLOG_NEW;
}


STATIC void
_udi_env_log_written(udi_cb_t *gcb,
		     udi_status_t correlated_status)
{
	_udi_MA_logcb_t *logp;

	logp = (_udi_MA_logcb_t *) gcb->scratch;
	UDI_ENQUEUE_TAIL(&_udi_MA_logcbQ, &logp->logcb_q);
}

void
_udi_env_log_write(udi_trevent_t trace_event,
		   udi_ubit8_t severity,
		   udi_index_t meta_idx,
		   udi_status_t original_status,
		   udi_ubit16_t msgnum,
		   ...)
{
	udi_cb_t *gcb;
	_udi_MA_logcb_t *logp;
	udi_queue_t *cbq;
	va_list arg;

	_OSDEP_ASSERT(udi_trlog_running == TRUE);

	if (!_udi_MA_logcb_alloc_done && _udi_MA_driver) {
		/*
		 * Couldn't do this in _udi_trlog_init because the MA 
		 * wasn't started yet for the _udi_MA_cb_alloc operations, 
		 * but do it now.  This should be identical code to the 
		 * _udi_trlog_init.
		 */
		int ii;
		udi_MA_cb_t *cb;
		_udi_MA_logcb_t *logp;

		for (ii = 0; ii < _OSDEP_NUM_LOGCB; ii++) {
			cb = _udi_MA_cb_alloc();
			gcb = UDI_GCB(&cb->v.m);
			logp = (_udi_MA_logcb_t *) gcb->scratch;
			logp->logcb_cb = gcb;
			UDI_ENQUEUE_TAIL(&_udi_MA_logcbQ, &logp->logcb_q);
		}
		_udi_MA_logcb_alloc_done = TRUE;
	}

	if (!UDI_QUEUE_EMPTY(&_udi_MA_logcbQ)) {
		cbq = UDI_DEQUEUE_HEAD(&_udi_MA_logcbQ);
		logp = UDI_BASE_STRUCT(cbq, _udi_MA_logcb_t, logcb_q);
		gcb = logp->logcb_cb;

		va_start(arg, msgnum);
		_udi_log_vwrite(&_udi_env_log_written, gcb, trace_event,
				severity, meta_idx,
				original_status, msgnum, arg);
		va_end(arg);
	}
}



/* _udi_trlog_fmt
 *
 * This is the routine which formats the output trace or log
 * information in a standard fashion based on the parameters of the call.
 *
 * The primary activity performed here is a udi_strncat of the environment's
 * information with the driver's information.  This is somewhat inefficient,
 * but this is the daemon process so it doesn't have to be quite as efficient
 * and it does have the advantage that _OSDEP_TRACE_DATA and _OSDEP_LOG_DATA
 * will be called with a single buffer and not multiple times for the same
 * event.
 */

#define ENV_FMT_HDRLEN 132

STATIC int
_udi_trlog_fmt(char *outbuf,
	       char *trlog_type,
	       _udi_trlog_info_t *trlog_info,
	       _udi_channel_t *chan)
{
	_udi_driver_t *pdriver;
	_udi_dev_node_t *node;
	int outlen, newlen;
	char *bp;

	pdriver = chan->chan_region->reg_driver;
	node = chan->chan_region->reg_node;

	udi_strcpy(outbuf, trlog_type);
	outlen = udi_strlen(outbuf);
	bp = outbuf + outlen;

	*(bp++) = '.';
	outlen++;

	_OSDEP_TIMESTAMP_STR(bp, ENV_FMT_HDRLEN - outlen,
			     trlog_info->trlog_timestamp);
	newlen = udi_strlen(bp);
	outlen += newlen;
	bp += newlen;

	if (trlog_info->trlog_event & UDI_TREVENT_META_MASK)
		udi_snprintf(bp, ENV_FMT_HDRLEN - outlen, "-%s/%d",
			     pdriver->drv_name, trlog_info->trlog_meta_idx);
	else
		udi_snprintf(bp, ENV_FMT_HDRLEN - outlen, "-%s",
			     pdriver->drv_name);
	newlen = udi_strlen(bp);
	outlen += newlen;
	bp += newlen;

	if (node) {
		/*
		 * If we have identifying information, add it now. 
		 */
		udi_snprintf(bp, ENV_FMT_HDRLEN - outlen, "%d %s %s ",
			     node->instance_number,
			     node->attr_address_locator ? node->
			     attr_address_locator : "",
			     node->attr_physical_locator ? node->
			     attr_physical_locator : "");
		newlen = udi_strlen(bp);
		outlen += newlen;
		bp += newlen;
	}

	udi_snprintf(bp, ENV_FMT_HDRLEN - outlen, udi_trevent_name,
		     trlog_info->trlog_event);
	newlen = udi_strlen(bp);
	outlen += newlen;
	bp += newlen;

	if (udi_strcmp(trlog_type, "T") == 0) {
		*(bp++) = ':';
		*(bp++) = ' ';
		outlen += 2;
	} else {
		*(bp++) = '\n';
		outlen++;
	}

	/*
	 * Allow the user to complete the formatting of this entry 
	 */

	udi_strcpy(bp, trlog_info->trlog_data);	/* slow... */
	/*
	 * don't need strncpy because trlog_size should be OK... 
	 */

	return outlen + strlen(bp);
}


/* _udi_trlog_daemon_work
 *
 * This is the work routine called by the _udi_trlog_daemon whenever
 * that daemon thread wakes up and checks for more work to do.  */

udi_boolean_t
_udi_trlog_daemon_work(void)
{
	_udi_trlog_t *trlogp;
	static char fmtbuf[_OSDEP_TRLOG_LIMIT + ENV_FMT_HDRLEN];
	static char loghdr[ENV_FMT_HDRLEN];
	_udi_cb_t *cb;
	udi_queue_t lclhead;
	udi_size_t len;
	udi_boolean_t ready_to_exit = FALSE;

	while (!ready_to_exit) {

		for (;;) {		/* process trace ring buffer until no more valid entries */

			/*
			 * Get next entry on ring to be handled (if any) 
			 */

			_OSDEP_SIMPLE_MUTEX_LOCK(&_udi_trlog_mutex);

			if (_udi_trlog_consumer_idx == _udi_trlog_producer_idx) {
				/*
				 * Ring empty.
				 * Exit while holding _udi_trlog_mutex.
				 */
				break;
			}

			trlogp = &_udi_trlog_events[_udi_trlog_consumer_idx];

			if (trlogp->trlog_info.trlog_event == 0) {
				/*
				 * Not completed yet, don't process now.
				 * Exit while holding _udi_trlog_mutex.
				 */
				break;
			}

			_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_trlog_mutex);

			/*
			 * Now do system formatting of this entry. 
			 */

			len = _udi_trlog_fmt(fmtbuf, "T",
					     &trlogp->trlog_info,
					     trlogp->trlog_region->
					     reg_first_chan);

			_OSDEP_SIMPLE_MUTEX_LOCK(&_udi_trlog_mutex);
			if (++_udi_trlog_consumer_idx == _OSDEP_TRLOG_SIZE)
				_udi_trlog_consumer_idx = 0;
			_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_trlog_mutex);

			/*
			 * Output this entry in the OS defined manner. 
			 */

			_OSDEP_TRACE_DATA(fmtbuf, len);
		}

		/*
		 * This point is reached when the trace ring buffer is empty.  
		 * Now check to see if any logging should be done. (Note: 
		 * we are still holding the _udi_trlog_mutex). 
		 */

		UDI_QUEUE_MOVE(&_udi_trlog_q, &lclhead);
		_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_trlog_mutex);

		while (!UDI_QUEUE_EMPTY(&lclhead)) {
			cb = (_udi_cb_t *)UDI_DEQUEUE_HEAD(&lclhead);

			/*
			 * correlate this status 
			 */
			if ((cb->cb_callargs.logm.
			     logm_status & UDI_CORRELATE_MASK) == 0) {
				cb->cb_callargs.logm.logm_status |=
					_udi_trlog_correlate;
				_udi_trlog_correlate +=
					(1 << UDI_CORRELATE_OFFSET);
				if ((_udi_trlog_correlate & UDI_CORRELATE_MASK)
				    == 0)
					_udi_trlog_correlate =
						(1 << UDI_CORRELATE_OFFSET);
			}

			/*
			 * Trace the event if it's not UDI_TREVENT_LOG type 
			 * and that trace class is enabled.
			 *
			 * Note that checking at this point may not 
			 * entirely correct * because the time delay 
			 * between the original udi_log_write call and 
			 * this code may have resulted in a change in the
			 * trace_mask.  This is deemed "implentation 
			 * dependent" at this point in time.
			 *
			 * Another note: it would be faster to just call
			 * _udi_trlog_fmt once and then re-adjust the env 
			 * header to output the formatted data twice (i.e. 
			 * don't format twice also).  This should be done 
			 * at some point... 
			 */

			len = _udi_trlog_fmt(fmtbuf, "T",
					     &cb->cb_callargs.logm.trlog_info,
					     _UDI_CB_TO_CHAN(cb));

			_OSDEP_TRACE_DATA(fmtbuf, len);

			/*
			 * Now log this event 
			 */
			udi_snprintf(loghdr, ENV_FMT_HDRLEN,
				     "\n====== %s #%d " UDI_STATUS_NAMES " ",
				     udi_log_severities[cb->cb_callargs.logm.
							logm_severity],
				     (cb->cb_callargs.logm.
				      logm_status & UDI_CORRELATE_MASK) >>
				     UDI_CORRELATE_OFFSET,
				     cb->cb_callargs.logm.
				     logm_status & UDI_SPECIFIC_STATUS_MASK);

			len = _udi_trlog_fmt(fmtbuf, loghdr,
					     &cb->cb_callargs.logm.trlog_info,
					     _UDI_CB_TO_CHAN(cb));


			/*
			 * fix _udi_do_callback and this could be removed; 
			 * a compile check really 
			 */
			_OSDEP_ASSERT((void *)&cb->cb_callargs.logm.
				      logm_status !=
				      (void *)&cb->cb_allocm.alloc_state);

			if (_OSDEP_LOG_DATA(fmtbuf, len, cb) == 0)
				_udi_queue_callback(cb);
		}


		/*
		 * Final check to see if there's any updates/completions in
		 * the trace ring that should be processed before we leave. 
		 */

		_OSDEP_SIMPLE_MUTEX_LOCK(&_udi_trlog_mutex);
		ready_to_exit = ((_udi_trlog_consumer_idx ==
				  _udi_trlog_producer_idx) ||
				 (_udi_trlog_events[_udi_trlog_consumer_idx].
				  trlog_info.trlog_event == 0));
		_OSDEP_SIMPLE_MUTEX_UNLOCK(&_udi_trlog_mutex);

	}

	return TRUE;
}


/*
 * Initialization of the UDI Tracing and Logging subsystem
 *
 * This must be called from the UDI environment initialization code
 * before any tracing or logging events occur.
 */

void
_udi_trlog_init(void)
{
	int ii;
	udi_MA_cb_t *cb;
	_udi_MA_logcb_t *logp;

	_udi_trlog_correlate = (1 << UDI_CORRELATE_OFFSET);

	_OSDEP_SIMPLE_MUTEX_INIT(&_udi_trlog_mutex,
				 "_udi_trlog_ring_buffer_lock");

	_udi_trlog_producer_idx = 0;
	_udi_trlog_consumer_idx = 0;

	UDI_QUEUE_INIT(&_udi_trlog_q);

	UDI_QUEUE_INIT(&_udi_MA_logcbQ);
	_udi_MA_logcb_alloc_done = FALSE;
	if (_udi_MA_driver) {
		udi_cb_t *gcb;

		for (ii = 0; ii < _OSDEP_NUM_LOGCB; ii++) {
			cb = _udi_MA_cb_alloc();
			gcb = UDI_GCB(&cb->v.m);
			logp = (_udi_MA_logcb_t *) gcb->scratch;
			logp->logcb_cb = gcb;
			UDI_ENQUEUE_TAIL(&_udi_MA_logcbQ, &logp->logcb_q);
		}
		_udi_MA_logcb_alloc_done = TRUE;
	}
	/*
	 * else must do this later when MA is started 
	 */
	_udi_trlog_initial = 0;
	for (ii = 0; ii < 256; ii++)
		_udi_trlog_ml_initial[ii] = 0;

	udi_trlog_running = TRUE;
	_OSDEP_TRLOG_INIT;		/* spawn a thread, open files, whatever... */
}


void
_udi_trlog_deinit(void)
{
	udi_cb_t *gcb;
	_udi_MA_logcb_t *logp;
	udi_queue_t *cbq;

	_udi_trlog_daemon_work();	/* flush if possible */

	udi_trlog_running = FALSE;

	while (!UDI_QUEUE_EMPTY(&_udi_MA_logcbQ)) {
		/*
		 * otherwise discard 
		 */
		cbq = UDI_DEQUEUE_HEAD(&_udi_MA_logcbQ);
		logp = UDI_BASE_STRUCT(cbq, _udi_MA_logcb_t, logcb_q);
		gcb = logp->logcb_cb;
		udi_cb_free(gcb);
	}

	_OSDEP_TRLOG_DONE;

	_OSDEP_SIMPLE_MUTEX_DEINIT(&_udi_trlog_mutex);
}
