/*
 * File: linux/env_test.c
 *
 * Unit test the Linux UDI environment.
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

/******************************************************************************
 * Abstract: This file is used to unit test the Linux UDI environment. It
 *		builds env_test.o which can then be loaded with modprobe to
 *		test the Linux UDI environment.
 *****************************************************************************/
/*
 * This is the only file in this module that will contain the version info,
 * so remove the __NO_VERSION__ macro.
 */
#undef __NO_VERSION__

#include "udi_osdep_opaquedefs.h"
#include <udi_env.h>
#include <linux/delay.h>
#include <linux/kmod.h>

/*
 * Gratuitous persistent module property.
 * The design of Linux kernel module persistent parameter data doesn't allow
 * for us to create new parameters at module runtime. Therefore, we cannot
 * use kernel module persistent parameters as UDI persistent propery storage.
 * Also, we cannot configure these parameters on a per-device basis. UDI
 * desires this functionality, so an in-kernel property manager must be
 * used.
 */
#define PARM_STR_SIZE 65
char parm_str[PARM_STR_SIZE];
MODULE_PARM(parm_str, "c" __MODULE_STRING(PARM_STR_SIZE) "p");

/*
 * Define macros for pass and fail
 */
#define PASS 0
#define FAIL 1

static const char* pass_str = "PASS\t";
static const char* fail_str = "FAIL\t";

/*
 * Declare the prototype that all test functions will use.
 * A return value of 0 indicates success. Non-zero indicates failure.
 */
typedef int (*TEST_FUNC)(void);


/*
 * Declare prototypes for each of the test functions.
 */
STATIC int timer_services( void );
STATIC int envdebug_services( void );

#if NOTYET
STATIC int mutex_services( void );
STATIC int semaphore_services( void );
STATIC int event_services( void );

/* Test DMA by allocating memory of various constraints */
STATIC int dma_services( void );

/* Test malloc by overwritting boundaries */
/*
 * This requires upgrading this test harness to provide a kernel_thread
 * per-test case.
 */
STATIC int malloc_services( void );

/* Move POSIX transtest and piotest into kernel-land */
STATIC int pio_services( void );
#endif

/*
 * Declare the array of all of the test functions. This array is iterated
 * and each test is called in order.
 */
static TEST_FUNC tests[] =
{
    timer_services,
    envdebug_services
};

/******************************************************************************
 * Function: _start_tests()
 *
 * Abstract: This is the entry point for the UDI environment tests. This
 *		function is started by kernel_thread to execute the tests. It
 *		loops through the "tests" array calling each of the testing
 *		functions and reporting their status in dmesg.
 *****************************************************************************/
STATIC int _start_tests( void* unused )
{
    int result = 0;
    int i = 0;

    /*
     * Mark the module as busy so we can't be unloaded while the tests are
     * running.
     */
    MOD_INC_USE_COUNT;

    /* Call each of the test functions in the list. */
    for( i = 0; i < sizeof(tests)/sizeof(tests[0]); i++ )
    {
        printk( "Starting test %d\n", i );
        printk( "================\n" );
        result = tests[i]();


        /*
         * A return value of 0 indicates success. Output the results of the
         * group of tests.
         */
        if( FAIL == result )
        {
            printk( "*** Test %d FAILED\n", i );
        }
        else
        {
            printk( "Test %d passed\n", i );
        }
    }

    /*
     * Now that we're done running tests, release the reference count we
     * grabbed earlier.
     */
    MOD_DEC_USE_COUNT;

    return( 0 );
}

STATIC
int envdebug_services( void )
{
    /*
     * Before firing off any tests, see if ASSERTs by the environment
     * get handled properly. We currently aren't tracking which kernel
     * thread was killed by an Oops, so, it's not really useful to
     * do this since it has a high potential for causing system
     * instability on Linuxen that do not handle panics like Oops's.
     * In a UDI driver, the offending driver will get region_kill'ed.
     */
    /* printk( "Testing ASSERT macro\n" );
     * ASSERT( 0 && "ASSERT forced." );
     * _OSDEP_ASSERT( 0 & "_OSDEP_ASSERT forced.");
     * udi_assert( 0 & "udi_assert forced.");
     * ---disabled... this should be done from a new kernel thread.
     */
    return 0;
}

STATIC void
kern_delay(unsigned long ticks)
{
#if 0
    unsigned long later;

    later = jiffies + (1 * HZ);
    while (jiffies < later) {
	schedule();
    }
#else
    /*udelay(1000000);*/ /* microsecond delay. */
    /*
     * Large delays are best done with mdelay,
     * which is a loop of udelay(1000)'s.
     */
    mdelay(1000); /* millisecond delay. */
#endif
}

STATIC 
int timer_services( void )
{
    int result = PASS;			/* ...we hope! */
    udi_timestamp_t initial_time;
    udi_timestamp_t finish_time;
    udi_time_t interval;
    const char *result_str = 0;

    /* 
     * Start by testing the timestamp services. Get the current time, sleep
     * for a second, get the next time, then compare the time between and the
     * time since the first call to udi_time_current.
     */
    printk( "Getting initial time\t(udi_time_current)\n" );
    initial_time = udi_time_current();

    printk( "Waiting 1 second...\n" );
    kern_delay(1*HZ);

    printk( "Getting finish time\t(udi_time_current)\n" );
    finish_time = udi_time_current();

    printk( "Calculating the duration between times\t(udi_time_between)\n" );
    interval = udi_time_between( initial_time, finish_time );

    if( initial_time >= finish_time )
    {
        result_str = fail_str;
        result = FAIL;
    }
    else
    {
        result_str = pass_str;
    }
    printk("%sChecking that initial time was before finish time\n", result_str);


    if( 1 != interval.seconds )
    {
        result_str = fail_str;
        result = FAIL;
    }
    else
    {
        result_str = pass_str;
    }
    printk("%sChecking that the time interval was 1 second\n", result_str);


    printk( "Getting initial time again\t(udi_time_current)\n" );
    initial_time = udi_time_current();

    printk( "Waiting 1 second...\n" );
    kern_delay(1*HZ);

    printk( "Calculating elapsed time since initial time\t(udi_time_since)\n" );
    interval = udi_time_since( initial_time );

    if( 1 != interval.seconds )
    {
        result_str = fail_str;
        result = FAIL;
    }
    else
    {
        result_str = pass_str;
    }
    printk("%sChecking that initial time was before finish time\n", result_str);



    /*
     * Now we are going to start testing other functions that require control
     * blocks. We are emulating the Management Agent at this point.
    _udi_MA_init();
     */

    return (result);
}



/******************************************************************************
 * Function: init_module()
 *
 * Abstract: This is the entry point for the env_test.o loadable module. To
 *		automate testing, this function will fire off the tests after
 *		making sure that the environment is loaded.
 *****************************************************************************/
int init_module( void )
{
    int result = 0;
    int pid = 0;	/* pid for test thread */

    /*
     * Print a huge marker into dmesg that makes it much easier for us to
     * see where each batch of new tests starts.
     */
    printk( "\n\n************************\n" );
    printk( "** Starting new tests **\n" );
    printk( "************************\n" );


    /* Make sure that the UDI environment is loaded */

	/*
	 * When the kernel is compiled without kernel module
	 * support (aka. #undef CONFIG_KMOD) the replacement
	 * version of request_module is a "do {} while(0)" loop.
	 * That is unacceptable and cannot be used as a rvalue.
	 */
#ifdef CONFIG_KMOD
    result = request_module( "udi_MA" );
#endif

    /*
     * Fire off a thread to run the tests and return immediately. This way the
     * kernel sees that we initialized correctly even if the tests barf. We
     * should still be able to remove a dead module as long as we were
     * initialized.
     */
    pid = kernel_thread( _start_tests, NULL, CLONE_FS);
    if (pid < 0)
    {
            printk(KERN_ERR "Failed to fork the test functions\n");
            return pid;
    }

    return 0;
}

/******************************************************************************
 * Function: cleanup_module()
 *
 * Abstract: This is the exit point for the env_test.o loadable module.
 *****************************************************************************/
void cleanup_module( void )
{
}

