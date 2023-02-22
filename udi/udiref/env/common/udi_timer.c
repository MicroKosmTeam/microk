
/*
 * File: env/common/udi_timer.c
 *
 * UDI Timer Services and Functions.
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

typedef struct {
	_OSDEP_MUTEX_T lock;
	udi_boolean_t heartbeat_running;
	udi_ubit32_t last_time_run;
	udi_ubit32_t long_elapsed;	/* Time since we last checked long_q */
	udi_queue_t short_q;
	udi_queue_t long_q;
	_OSDEP_TIMER_T heartbeat_timer_id;
} _udi_timer_t;

_udi_timer_t *_udi_timer_info;

STATIC void _udi_timer_heartbeat(_udi_timer_t *timer_info);

/* 
 * This is typically called from _udi_alloc_init. 
 */
void
_udi_timer_init(void)
{
	_udi_timer_info = _OSDEP_MEM_ALLOC(sizeof (*_udi_timer_info),
					   0, UDI_WAITOK);
	_OSDEP_MUTEX_INIT(&_udi_timer_info->lock, "_udi_timer_lock");
	UDI_QUEUE_INIT(&_udi_timer_info->short_q);
	UDI_QUEUE_INIT(&_udi_timer_info->long_q);
}

void
_udi_timer_deinit(void)
{
	/*
	 * TODO: cancel timer.
	 * _OSDEP_TIMER_CANCEL(...)
	 */
	_OSDEP_MUTEX_DEINIT(&_udi_timer_info->lock);
	_OSDEP_MEM_FREE(_udi_timer_info);
}

STATIC udi_ubit32_t
_udi_timer_repeating_recalc(_udi_cb_t *cb,
			    udi_ubit32_t *my_ticks_left)
{
	udi_sbit32_t ticks_left, ticks_beyond;
	udi_ubit32_t curr_time, start_time, interval_ticks, nmissed;

	curr_time = _OSDEP_CURR_TIME_IN_TICKS;
	interval_ticks = cb->cb_allocm._u.timer_request.interval_ticks;
	start_time = cb->cb_allocm._u.timer_request.start_time;

	/*
	 * Increment start_time to the beginning of the next interval. 
	 */
	start_time += interval_ticks;
	/*
	 * Calculate how far we are beyond the start of next interval. 
	 */
	ticks_beyond = curr_time - start_time;
	/*
	 * Note:  the only way ticks_beyond can be negative here as the
	 * result of this modulo-2^32 unsigned subtraction is if curr_time
	 * is more than 0x7fffffff ticks "ahead of" start_time, modulo 2^32,
	 * (over half a year in 100Hz ticks).  Or if the timer popped early
	 * (ie. curr_time really is behind the new start_time).  Neither
	 * should be occuring.  See the little example test program,
	 * timer-calc.c, for an example of how this works. 
	 */
	_OSDEP_ASSERT(ticks_beyond >= 0);

	/*
	 * Calculate nmissed and further update start_time and ticks_beyond
	 * as needed.
	 */
	nmissed = 0;
	while (ticks_beyond > interval_ticks) {
		nmissed++;
		ticks_beyond -= interval_ticks;
		start_time += interval_ticks;
	}
	cb->cb_allocm._u.timer_request.start_time = start_time;

	/*
	 * Calculate ticks_left to end of the next interval.
	 */
	ticks_left = interval_ticks - ticks_beyond;
	*my_ticks_left = ticks_left;

	return nmissed;
}

udi_ubit32_t
_udi_timer_start(udi_timer_expired_call_t *callback,
		 _udi_cb_t *cb,
		 udi_ubit32_t ticks,
		 _udi_timer_start_caller_t caller)
{
	_udi_timer_t *timer_info = _udi_timer_info;
	udi_ubit32_t curr_time, nmissed = 0;

	/*
	 * Fill in the timer-specific portions of the control block 
	 */
	cb->cb_flags |= _UDI_CB_CALLBACK;
	cb->cb_func = (_udi_callback_func_t *)callback;
	cb->cb_allocm.alloc_state = _UDI_ALLOC_TIMER;

	_OSDEP_MUTEX_LOCK(&timer_info->lock);

	curr_time = (udi_ubit32_t)_OSDEP_CURR_TIME_IN_TICKS;

	if (caller != _UDI_TIMER_START) {
		if (caller == _UDI_TIMER_START_REPEATING) {
			/*
			 * Starting repeating timer. 
			 */
			cb->cb_allocm._u.timer_request.start_time = curr_time;
		} else {
			/*
			 * Restarting repeating timer. Recalculate stuff. 
			 */
			nmissed = _udi_timer_repeating_recalc(cb, &ticks);
		}
	}
	/*
	 * To be synced up properly with _udi_timer_heartbeat we need
	 * to make ticks indicate an interval that starts from when
	 * the heartbeat last ran, so make that adjustment here.
	 */
	ticks += (timer_info->heartbeat_running) ?
		(curr_time - timer_info->last_time_run) : 0;

	/*
	 * Put the timer on the appropriate timeout list
	 */
	if (ticks > _OSDEP_LONG_TIMER_THRESHOLD) {
		/*
		 * Adjust ticks to reflect where we are in the heartbeat
		 * wrt checking the long list.  Every time we check the
		 * long list we decrement each of the ticks_left values
		 * by the long_elapsed value.  If, for example, ticks
		 * is 200 (2 seconds) and long_elapsed is currently 99
		 * (meaning that on the next tick we're going to check
		 * the long timeout list) then without this adjustment
		 * ticks_left would get decremented from 200 to 100
		 * even tho' only 1 tick elapsed.
		 */
		ticks += timer_info->long_elapsed;
		UDI_ENQUEUE_TAIL(&timer_info->long_q, &cb->cb_qlink);
	} else {
		UDI_ENQUEUE_TAIL(&timer_info->short_q, &cb->cb_qlink);
	}

	cb->cb_allocm._u.timer_request.ticks_left = ticks;

	/*
	 * Start the heartbeat if it's not currently running.
	 */
	if (timer_info->heartbeat_running) {
		_OSDEP_MUTEX_UNLOCK(&timer_info->lock);
		return nmissed;
	}
	timer_info->heartbeat_running = TRUE;
	/*
	 * Initialize last_time_run to the current time so we start
	 * off on the right foot in the heartbeat routine. 
	 */
	timer_info->last_time_run = curr_time;
	timer_info->long_elapsed = 0;
	_OSDEP_MUTEX_UNLOCK(&timer_info->lock);
	timer_info->heartbeat_timer_id =
		_OSDEP_TIMER_START(_udi_timer_heartbeat, timer_info,
				   _OSDEP_SHORT_TIMER_THRESHOLD, FALSE);
	return nmissed;
}

void
udi_timer_start(udi_timer_expired_call_t *callback,
		udi_cb_t *gcb,
		udi_time_t interval)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);

	/* Handle a 0 ns timer synchronously */
	if ((interval.seconds == 0) && (interval.nanoseconds == 0)) {
		callback(gcb);
		return;
	}
	cb->op_idx = _UDI_CT_TIMER_START;
	(void)_udi_timer_start(callback, cb, _OSDEP_TIME_T_TO_TICKS(interval),
			       _UDI_TIMER_START);
}

void
udi_timer_start_repeating(udi_timer_tick_call_t *callback,
			  udi_cb_t *gcb,
			  udi_time_t interval)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	udi_ubit32_t ticks = _OSDEP_TIME_T_TO_TICKS(interval);

	/* 0 ns repeating timers are not allowed */
	_OSDEP_ASSERT(ticks > 0);
	cb->op_idx = _UDI_CT_TIMER_START_REPEATING;
	cb->cb_allocm._u.timer_request.interval_ticks = ticks;

	(void)_udi_timer_start((udi_timer_expired_call_t *)callback,
			       cb, ticks, _UDI_TIMER_START_REPEATING);
}

/*
 * It would be nice to be able to reduce udi_timer_cancel to simply
 * 
 *   udi_timer_cancel(udi_cb_t *gcb)
 *   {
 *      _OSDEP_MUTEX_LOCK(cb->cb_allocm._u.timer_request.q_lock);
 *      UDI_QUEUE_REMOVE(&cb->cb_qlink);
 *      _OSDEP_MUTEX_UNLOCK(cb->cb_allocm._u.timer_request.q_lock);
 *      cb->cb_allocm->alloc_state = _UDI_ALLOC_IDLE;
 *      cb->cb_flags &= ~_UDI_CB_CALLBACK;
 *   }
 * 
 * where q_lock is set by timer_start to &timer_info->lock, and is
 * changed by the heartbeat routine to &region->reg_mutex before
 * putting the timer in the region queue.
 * 
 * But I don't think we can do this since (1) the two locks are
 * different types, SIMPLE_MUTEX_T vs MUTEX_T (which, for OSes that
 * need the distinction, the two mutex types are needed because the
 * region lock (simple) is obtained while holding timer_info->lock),
 * and (2) we need to hold the timer_info->lock before we can 
 * reliably grab q_lock, defeating the purpose of having q_lock.
 * So I don't think we can simplify to this level.  If we could,
 * we'd also be able to remove the need for _UDI_ALLOC_TIMER_DONE.
 */
void
udi_timer_cancel(udi_cb_t *gcb)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);
	_udi_alloc_marshal_t *allocm = &cb->cb_allocm;
	_udi_timer_t *timer_info = _udi_timer_info;

	_OSDEP_MUTEX_LOCK(&timer_info->lock);
	if (allocm->alloc_state == _UDI_ALLOC_TIMER) {
		/*
		 * Remove timer from short_q/long_q 
		 */
		UDI_QUEUE_REMOVE(&cb->cb_qlink);
	} else {
		/*
		 * Remove timer from region queue 
		 */
		_udi_region_t *region = _UDI_GCB_TO_CHAN(gcb)->chan_region;

		_OSDEP_ASSERT(allocm->alloc_state == _UDI_ALLOC_TIMER_DONE);
		REGION_LOCK(region);
		_UDI_REGION_Q_REMOVE(region, &cb->cb_qlink);
		REGION_UNLOCK(region);
	}
	_OSDEP_MUTEX_UNLOCK(&timer_info->lock);
	allocm->alloc_state = _UDI_ALLOC_IDLE;
	cb->cb_flags &= ~_UDI_CB_CALLBACK;
}

STATIC void
_udi_timer_heartbeat(_udi_timer_t *timer_info)
{
	udi_sbit32_t ticks_passed;
	udi_ubit32_t curr_time;
	udi_queue_t *elem, *tmp;
	udi_boolean_t reschedule_heartbeat = TRUE;
	_udi_region_t *region;
	udi_queue_t my_sched_q;

	UDI_QUEUE_INIT(&my_sched_q);


	_OSDEP_MUTEX_LOCK(&timer_info->lock);

	/*
	 * Use curr_time to prevent drift. 
	 */
	curr_time = (udi_ubit32_t)_OSDEP_CURR_TIME_IN_TICKS;
	ticks_passed = curr_time - timer_info->last_time_run;
	_OSDEP_ASSERT(ticks_passed >= 0);
	timer_info->last_time_run = curr_time;

	/*
	 * Process short timer list 
	 */
	UDI_QUEUE_FOREACH(&timer_info->short_q, elem, tmp) {
		_udi_cb_t *cb = (_udi_cb_t *)elem;
		_udi_alloc_marshal_t *allocm = &cb->cb_allocm;

		if ((allocm->_u.timer_request.ticks_left -= ticks_passed) <= 0) {
			/*
			 * Move to reqion queue 
			 */
			allocm->alloc_state = _UDI_ALLOC_TIMER_DONE;
			UDI_QUEUE_REMOVE(&cb->cb_qlink);

			region = _UDI_CB_TO_CHAN(cb)->chan_region;
			REGION_LOCK(region);
			_UDI_REGION_Q_APPEND(region, &cb->cb_qlink);
			REGION_UNLOCK(region);

			/*
			 * If we're moving more than one timer, be sure
			 * to not stick the same region on my_sched_q 
			 * multiple times.
			 */
			if (region->reg_alt_sched_link.next == NULL) {
				UDI_ENQUEUE_TAIL(&my_sched_q,
						 &region->reg_alt_sched_link);
			}
		}
	}

	/*
	 * Process long timer list 
	 */
	timer_info->long_elapsed += ticks_passed;
	if (timer_info->long_elapsed >= _OSDEP_LONG_TIMER_THRESHOLD) {
		UDI_QUEUE_FOREACH(&timer_info->long_q, elem, tmp) {
			_udi_cb_t *cb = (_udi_cb_t *)elem;
			_udi_alloc_marshal_t *allocm = &cb->cb_allocm;

			if ((allocm->_u.timer_request.ticks_left -=
			     timer_info->long_elapsed) <=
			    _OSDEP_LONG_TIMER_THRESHOLD) {
				/*
				 * Move to short_q 
				 */
				UDI_QUEUE_REMOVE(&cb->cb_qlink);
				UDI_ENQUEUE_TAIL(&timer_info->short_q,
						 &cb->cb_qlink);
			}
		}
		timer_info->long_elapsed = 0;
	}

	/*
	 * Reschedule timer heartbeat if there are timers left to handle 
	 */
	if (UDI_QUEUE_EMPTY(&timer_info->short_q) &&
	    UDI_QUEUE_EMPTY(&timer_info->long_q)) {
		/*
		 * No timers left to handle 
		 */
		reschedule_heartbeat = FALSE;
		timer_info->heartbeat_running = FALSE;
		/*
		 * Set long_elapsed to zero so we don't adjust ticks 
		 * improperly on the next udi_timer_start. 
		 */
		timer_info->long_elapsed = 0;
	}

	_OSDEP_MUTEX_UNLOCK(&timer_info->lock);

	/*
	 * Run any regions on my_sched_q.
	 * 
	 * Note: On a repeating timer, just before calling the driver's
	 * callback (in _udi_do_callback) we'll move the cb back to the
	 * appropriate global timeout list (short_q/long_q).
	 */
	while (!UDI_QUEUE_EMPTY(&my_sched_q)) {
		elem = UDI_DEQUEUE_HEAD(&my_sched_q);
		region = UDI_BASE_STRUCT(elem, _udi_region_t,
					 reg_alt_sched_link);

		region->reg_alt_sched_link.next = NULL;
		_udi_run_inactive_region(region);
	}

	if (reschedule_heartbeat) {
		timer_info->heartbeat_timer_id =
			_OSDEP_TIMER_START(_udi_timer_heartbeat, timer_info,
					   _OSDEP_SHORT_TIMER_THRESHOLD,
					   FALSE);
	}
}

udi_timestamp_t
udi_time_current(void)
{
	udi_timestamp_t ct;

	_OSDEP_GET_CURRENT_TIME(ct);

	return (ct);
}

udi_time_t
udi_time_between(udi_timestamp_t start_time,
		 udi_timestamp_t end_time)
{
	udi_time_t diff;

	_OSDEP_SUBTRACT_TIME(end_time, start_time, diff);
	return diff;
}

udi_time_t
udi_time_since(udi_timestamp_t start_time)
{
	udi_timestamp_t now;
	udi_time_t result;

	_OSDEP_GET_CURRENT_TIME(now);
	_OSDEP_SUBTRACT_TIME(now, start_time, result);

	return result;
}
