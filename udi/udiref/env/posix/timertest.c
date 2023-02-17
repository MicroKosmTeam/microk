
/*
 * File: env/posix/timertest.c
 *
 * Verify that simple timer plumbing works.   
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <udi_env.h>
#include <posix.h>

#if defined(PTHREAD_SIGMASK_UNAVAILABLE)
/*
 * We don't want sleep or usleep to be woken up by setitimer's SIGALRMs!
 * Not all OS's can handle this, since some may implement [u]sleep
 * with SIGALRM, conflicting with setitimer's alarms.
 * Darwin apparently doesn't share SIGALRMs, but its sleep will return
 * EINTR if woken up by a SIGALRM that setitimer generated.
 * Other OS's may use select instead of sleep or usleep to workaround
 * the sharing of SIGARLM, or the lack of pthread_sigmask.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

/*
 * This sleep is not prone to being interrupted early by setitimer's SIGALRMs.
 */
static unsigned int my_sleep(unsigned int sleeptime)
{
	unsigned int result;
	sigset_t block, oldset;
	sigemptyset(&block);
	sigaddset(&block, SIGALRM);
	udi_assert(sigprocmask(SIG_BLOCK, &block, &oldset) == 0);
	result = sleep(sleeptime);
	sigsuspend(&oldset); /* service one blocked signal */
	udi_assert(sigprocmask(SIG_UNBLOCK, &block, &oldset) == 0);
	return result;
}

static void my_usleep(unsigned int sleeptime)
{
	sigset_t block, oldset;
	sigemptyset(&block);
	sigaddset(&block, SIGALRM);
	udi_assert(sigprocmask(SIG_BLOCK, &block, &oldset) == 0);
	usleep(sleeptime);
	sigsuspend(&oldset); /* service one blocked signal */
	udi_assert(sigprocmask(SIG_UNBLOCK, &block, &oldset) == 0);
}
#define sleep(x) my_sleep(x)
#define usleep(x) my_usleep(x)
#endif

#define NTIMERS 255

#define MSEC_TO_NSEC(x) ((x) * USEC_PER_SEC)

static udi_timestamp_t start_test;
STATIC volatile int timer_ticks;
STATIC int timers_cancelled;
STATIC int timers_expired;
STATIC int full_test;
STATIC volatile int timer_max_ticks;

STATIC udi_cb_t *repeating_cb;

_OSDEP_EVENT_T timer_ev;

void cballoc_repeating(udi_cb_t *,
		       udi_cb_t *);
void cballoc_complete1(udi_cb_t *,
		       udi_cb_t *);
void cballoc_final(udi_cb_t *);

void
timerexpired_cb(udi_cb_t *gcb)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);

	timers_expired++;
	printf("Timer expired gcb: %p\n", gcb);
	assert(cb->cb_allocm.alloc_state == _UDI_ALLOC_IDLE);
}

void
final_timerexpired_cb(udi_cb_t *gcb)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);

	timers_expired++;
	printf("Final Timer expired gcb: %p\n", gcb);
	assert(cb->cb_allocm.alloc_state == _UDI_ALLOC_IDLE);
	_OSDEP_EVENT_SIGNAL(&timer_ev);
}

void
timertick_cb(void *context,
	     udi_ubit32_t nmissed)
{
	printf(".");
	fflush(stdout);
	timer_ticks++;
	if (timer_ticks >= timer_max_ticks) {
		_OSDEP_EVENT_SIGNAL(&timer_ev);
	}
}

void
do_cancel(udi_cb_t *gcb)
{
	_udi_cb_t *cb = _UDI_CB_TO_HEADER(gcb);

	assert(cb->cb_allocm.alloc_state != _UDI_ALLOC_IDLE);
	udi_timer_cancel(gcb);
	assert(cb->cb_allocm.alloc_state == _UDI_ALLOC_IDLE);
	assert(0 == (cb->cb_flags & _UDI_CB_CALLBACK));
	timers_cancelled++;
	printf("Timer cancelled\n");
	/*
	 * Release CB 
	 */
	udi_cb_free(gcb);
}

void cballoc_stage1(udi_cb_t *);

void
cballoc(udi_cb_t *gcb,
	udi_cb_t *new_cb)
{
	udi_time_t t, tdone;
	udi_timestamp_t tstart;

	t.seconds = 3;
	t.nanoseconds = 1000;

	tstart = udi_time_current();
	udi_timer_start(timerexpired_cb, new_cb, t);
	udi_assert(timers_cancelled == 0);
	udi_assert(timers_expired == 0);
	usleep(1500 * 1000);
	tdone = udi_time_since(tstart);
	printf("Tested udi_timer_start on 1.5 seconds:           "
	       "%d.%09d seconds\n", tdone.seconds, tdone.nanoseconds);

	/*
	 * Allow some timer slop 
	 */
	udi_assert(tdone.seconds == 1);
	udi_assert(tdone.nanoseconds > MSEC_TO_NSEC(400));
	udi_assert(tdone.nanoseconds < MSEC_TO_NSEC(800));

	/*
	 * Wait for the 3 second timer to expire 
	 */
	udi_assert(timers_expired == 0);
#if 0
	usleep(2000 * 1000);
	udi_assert(timers_expired == 1);

	udi_cb_alloc(cballoc_repeating, gcb, 1, _udi_MA_chan);

	udi_cb_free(new_cb);
#else
	t.seconds = 2;
	t.nanoseconds = MSEC_TO_NSEC(500);
	_OSDEP_TIMER_START(cballoc_stage1, new_cb, _OSDEP_TIME_T_TO_TICKS(t),
			   FALSE);
	return;
#endif
}

void
cballoc_stage1(udi_cb_t *new_cb)
{
	udi_assert(timers_expired == 1);

	udi_cb_free(new_cb);

	/*
	 * Start the next test off... 
	 */
	_OSDEP_EVENT_SIGNAL(&timer_ev);
}

/*
 * Repeating timer tests...
 */
void
cballoc_repeating(udi_cb_t *gcb,
		  udi_cb_t *new_cb)
{
	udi_time_t t;

	repeating_cb = new_cb;

	if (full_test) {
		printf("Starting five ticks one second apart:");
		fflush(stdout);
		t.seconds = 1;
		t.nanoseconds = 0;
		timer_max_ticks = 5;
		start_test = udi_time_current();
		udi_timer_start_repeating(timertick_cb, new_cb, t);

	} else {
		printf("Starting ten ticks one tenth of a second apart:");
		fflush(stdout);
		t.seconds = 0;
		t.nanoseconds = MSEC_TO_NSEC(100);
		timer_max_ticks = 10;
		start_test = udi_time_current();
		udi_timer_start_repeating(timertick_cb, new_cb, t);
	}
	return;
}

void
cballoc_complete(udi_cb_t *gcb,
		 udi_cb_t *new_cb)
{
	udi_time_t t;
	extern _udi_channel_t *_udi_MA_chan;

	/*
	 * Start a long timer and be sure we can cancel it. 
	 */
	t.seconds = 30;
	t.nanoseconds = 1000;		/* and 1 us */
#if 0
	timers_expired = 0;		/* Reset */
#endif
	printf("Starting 30 second timer:");
	fflush(stdout);
	udi_timer_start(timerexpired_cb, new_cb, t);

	printf("Cancelling timer:");
	fflush(stdout);
	do_cancel(new_cb);
	/*
	 * This one shouldn't have expired 
	 */
	udi_assert(timers_expired == 1);

	/*
	 * new_cb is now history, we need to allocate another one to complete
	 * our tests
	 */
	udi_cb_alloc(cballoc_complete1, gcb, 1, _udi_MA_chan);
}

void
cballoc_complete1(udi_cb_t *gcb,
		  udi_cb_t *new_cb)
{
	udi_time_t t;

	t.seconds = 0;
	t.nanoseconds = 1234;
	printf("Starting short timer:");
	fflush(stdout);
	udi_timer_start(timerexpired_cb, new_cb, t);

	/*
	 * If we're running with a callback limit of 1 [POSIX_CALLBACK_LIMIT]
	 * we will not have had the timer scheduled as the region is busy
	 * running this routine.
	 * To get around this we use the _OSDEP_TIMER_START() to schedule the
	 * check for timer completion, and tidy up the remaining storage.
	 */
	t.seconds = 0;
	t.nanoseconds = MSEC_TO_NSEC(50);
	printf("Starting 50 ms timer\n");
	_OSDEP_TIMER_START(cballoc_final, new_cb, _OSDEP_TIME_T_TO_TICKS(t),
			   FALSE);
	return;
}

void
multi_timer_got_cb(udi_cb_t *gcb,
		   udi_cb_t *first_cb)
{
	udi_cb_t *cb;
	udi_cb_t *ncb;
	udi_time_t t;
	int i = 0;

	t.seconds = 0;
	t.nanoseconds = 1000;

	for (cb = first_cb; cb; cb = cb->initiator_context) {
		i++;
		t.nanoseconds += 500 * 1000;
		/*
		 * nanosec was 50000, but now they aren't all 1 tick long 
		 */
		if (i & 1)
			udi_timer_start(timerexpired_cb, cb, t);
		else
			udi_timer_start_repeating(timertick_cb, cb, t);
	}
#if 1					/* Wait about 1.5 clock ticks */
	usleep(3 * (USEC_PER_SEC / POSIX_TIMER_HZ) / 2);
#endif

	/*
	 * Some will have fired, some will not have yet fired.
	 * Now cancel them all, just to be obnoxious.
	 */
	for (cb = first_cb; cb; cb = ncb) {
		ncb = cb->initiator_context;
		udi_timer_cancel(cb);
		udi_cb_free(cb);
		cb = ncb;
	}

	_OSDEP_EVENT_SIGNAL(&timer_ev);

}


void
cballoc_final(udi_cb_t *new_cb)
{
	/*
	 * This one _should_ have expired 
	 */
	udi_assert(timers_expired == 2);

	/*
	 * We've cancelled both the repeating and the manual timer. 
	 */
	udi_assert(timers_cancelled == 2);

	udi_cb_free(new_cb);

	/*
	 * Allow the main thread to continue 
	 */
	_OSDEP_EVENT_SIGNAL(&timer_ev);
}


void
timertest_parseargs(int c,
		    void *optarg)
{
	switch (c) {
	case 'f':
		full_test = 1;
	}
}


_OSDEP_EVENT_T test_ev;


int
main(int argc,
     char *argv[])
{
	extern _udi_channel_t *_udi_MA_chan;
	udi_limits_t *init_context_limits;
	udi_time_t t;
	udi_timestamp_t x, x2;
	static struct {
		_udi_cb_t _cb;
		udi_cb_t v;
	} _cb;
	extern unsigned int guess_clock_res(void);

	posix_init(argc, argv, "f", &timertest_parseargs);
	_OSDEP_EVENT_INIT(&timer_ev);

	x = udi_time_current();
	sleep(1);
	x2 = udi_time_current();

	t = udi_time_between(x, x2);
	printf("time_between stamps approx one second apart:     "
	       "%d.%09d seconds\n", t.seconds, t.nanoseconds);

	t = udi_time_between(x, x);
	_OSDEP_ASSERT(t.seconds == 0);
	_OSDEP_ASSERT(t.nanoseconds == 0);

	t = udi_time_since(x);
	printf("Time_since first stamp.  (Approx one second:     "
	       "%d.%09d seconds\n", t.seconds, t.nanoseconds);


	/*
	 * Now start timer service tests.
	 */

	_cb.v.channel = _udi_MA_chan;
	init_context_limits = &_udi_MA_chan->chan_region->reg_init_rdata->
		ird_init_context.limits;
	udi_cb_alloc(cballoc, &_cb.v, 1, _udi_MA_chan);

	/*
	 * Wait for the initial cballoc tests to complete
	 */
	_OSDEP_EVENT_WAIT(&timer_ev);

	/*
	 * Start the repeating timer tests...
	 */
	udi_cb_alloc(cballoc_repeating, &_cb.v, 1, _udi_MA_chan);


	/*
	 * Wait for the repeating timer test to complete. Once it has, run
	 * the remaining tests
	 */
	_OSDEP_EVENT_WAIT(&timer_ev);
	t = udi_time_since(start_test);
	printf("\nTime_since ticks started: %d.%09d seconds\n",
	       t.seconds, t.nanoseconds);
	printf("Cancelling repeating timer\n");
	do_cancel(repeating_cb);
	udi_cb_alloc(cballoc_complete, &_cb.v, 1, _udi_MA_chan);

	_OSDEP_EVENT_WAIT(&timer_ev);

	/*
	 * Start a trainload of timers, cancel them all.
	 */
	udi_cb_alloc_batch(multi_timer_got_cb, &_cb.v, 1, NTIMERS, FALSE, 0,
			   0);
	_OSDEP_EVENT_WAIT(&timer_ev);

	/*
	 * These values really should be _used_ in some of the math above.
	 * for now, just prove that the environment sets them. 
	 *
	 * There is some risk (especially if the system is non-idle when this
	 * is run) that we could guess a different resolution.   So if we 
	 * see intermittent failures here, we might want guess_clock_res
	 * to stash its answer in a global and test against that.
	 */
	udi_assert(init_context_limits->min_curtime_res == guess_clock_res());
	udi_assert(init_context_limits->min_timer_res ==
	       NSEC_PER_SEC / POSIX_TIMER_HZ);

	sleep(1);

	_OSDEP_EVENT_DEINIT(&timer_ev);

	printf("Deiniting posix environment\n");
	posix_deinit();

	/*
	 * Prove that the base MA stuff doesn't leak...  
	 */
	udi_assert(Allocd_mem == 0);

	printf("Exiting\n");
	return 0;

}
