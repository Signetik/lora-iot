/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

//#include <bsd.h>
#include <zephyr.h>
#include <logging/log.h>

#include <device.h>
#include <drivers/watchdog.h>

#include "signetik.h"
#include "vars.h"

#define	DEFAULT_WDT_NODE DT_ALIAS(wdt)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_WDT_NODE,	okay),
		 "No default watchdog Interface	specified in DT");
#define	DEFAULT_WDT	DT_LABEL(DEFAULT_WDT_NODE)

///	Zephyr Logging Module.
LOG_MODULE_REGISTER(wdt, CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

///	Constants

// Reset SoC after 20s if WDT is not fed. // @note The LTE thread loop takes 5+	seconds!
#define	WDT_TIMEOUT_MS	20000U			  // @todo Reduce if possible!

// All threads report in separately.
static uint8_t THREAD_MASK = 0;
static uint8_t num_threads = 0;
static uint8_t wdt_aggregate = 0;
// @note If	no threads are registered, the WDT will	operate	in "simple"	mode (always fed each loop).

/**	@brief Watchdog	Callback. */
static void	wdt_callback(struct	device *wdt_dev, int channel_id)
{
	static bool	handled_event;

	if (handled_event)
	{
		return;
	}

	wdt_feed(wdt_dev, channel_id);

	LOG_INF("Handled WDT callback! Ready to	reset!");
	handled_event =	true;
}

/****************************************
 *			 WDT Task Thread			*
 ****************************************/

void wdt_thread(void *p1, void *p2,	void *p3)
{
	int	err;
	int	wdt_channel_id;
	struct device *wdt;
	struct wdt_timeout_cfg wdt_config;

	LOG_INF("Starting Watchdog Timer (WDT) Thread.");

	wdt	= device_get_binding(DEFAULT_WDT);
	if (!wdt)
	{
		LOG_ERR("Cannot	get	WDT	device %s!", DEFAULT_WDT);
		return;
	}

	/* Reset SoC when watchdog timer expires. */
	wdt_config.flags = WDT_FLAG_RESET_SOC;

	/* Expire watchdog after x milliseconds. */
	wdt_config.window.min =	0U;
	wdt_config.window.max =	WDT_TIMEOUT_MS;

	/* Set up watchdog callback. Jump into it when watchdog	expired. */
	wdt_config.callback	= wdt_callback;

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	if (wdt_channel_id == -ENOTSUP)
	{
		/* IWDG	driver for STM32 doesn't support callback */
		wdt_config.callback	= NULL;
		wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	}
	if (wdt_channel_id < 0)
	{
		LOG_ERR("Watchdog install error	(%d)!",	wdt_channel_id);
		return;
	}

	err	= wdt_setup(wdt, 0);
	if (err	< 0)
	{
		LOG_ERR("Watchdog setup	error (%d)!", err);
		return;
	}

	/* Feeding watchdog	FOREVER. */
	LOG_INF("Feed Watchdog after all RTOS threads have reported	in!");
	while (1)
	{
		// Check all threads. @note	This is	always TRUE	until a	thread is registered!
		if (wdt_aggregate == THREAD_MASK)
		{
			// Please feed our dog.	Do not kick	it.	We like	our	animal friends.	:)
			LOG_DBG("Feed");

			// Feed	WDT
			wdt_feed(wdt, wdt_channel_id);

			// Reset aggregate for next	time.
			wdt_aggregate =	0;

			// Wait	1/2	of timeout period.
			k_sleep(K_MSEC(WDT_TIMEOUT_MS /	2));
		}
		else
		{
			// Check back sooner since all threads haven't reported	in yet.
			k_sleep(K_MSEC(WDT_TIMEOUT_MS /	10));
		}
	}
}

/**
 * @brief Feed the Watchdog	Timer.
 * @detail To be called	by *EVERY* running thread.
 */
void wdt_feed_watchdog(uint8_t thread_id)
{
	wdt_aggregate |= thread_id;
	LOG_DBG("Thread	ID [%X]	added to wdt_aggregate:	%X", thread_id,	wdt_aggregate);
}

/**
 * @ brief Register	RTOS Thread	with WDT.
 * @ return	Thread ID. Must	be used	when feeding WDT!!!
 */
uint8_t	wdt_register_thread(void)
{
	// Set new thread ID.
	uint8_t	new_thread_id =	(1 << num_threads);
	uint8_t	temp;
	
	// Check for open bit in thread	mask.
	for	(int i=0; i<=num_threads; i++) {
		temp = (1<<i);
		// If avaliable	bit	found, use that	for	the	ID.
		if ((THREAD_MASK & temp) ==	0) {
			 new_thread_id = temp;
			 break;
		 }
	}

	// Add ID to thread	mask.
	THREAD_MASK	|= new_thread_id;

	// Bump	thread count.
	num_threads++;

	LOG_INF("New Thread	ID:	%X", new_thread_id);
	LOG_INF("Thread	Mask:	%X", THREAD_MASK);

	// Return ID for RTOS thread to	use.
	return new_thread_id;
}

/**
 * @ brief Deregister RTOS Thread from WDT.
 * @ param thread_id ID	of the thread to remove.
 */
void wdt_deregister_thread(uint8_t thread_id)
{
// Do not deregister a thread that is not registered
		if ((THREAD_MASK & thread_id) != thread_id)
		{
			LOG_INF("Thread	ID:	%X NOT registered",	thread_id);
			LOG_INF("Thread	Mask:	%X", THREAD_MASK);
		}
		else
		{
			// Remove ID from thread mask.
			THREAD_MASK	&= ~(thread_id);

		// Decrement thread	count.
		num_threads--;

		// Reset aggregate to feed watchdog	this loop.
		// Next	loop uses updated thread mask.
		wdt_aggregate =	THREAD_MASK;

		LOG_INF("Thread	ID:	%X removed", thread_id);
		LOG_INF("Thread	Mask:	%X", THREAD_MASK);
	}
}

///	Create WDT thread/task.
K_THREAD_STACK_DEFINE(wdt_stack_area, WDT_STACKSIZE);
struct k_thread	wdt_thread_data;

///	Start WDT thread.
void wdt_thread_start(void)
{
	/* k_tid_t my_tid =	*/
	k_thread_create(&wdt_thread_data, wdt_stack_area,
		K_THREAD_STACK_SIZEOF(wdt_stack_area),
		wdt_thread,
		NULL, NULL,	NULL,
		WDT_PRIORITY, 0, K_NO_WAIT);
}

uint32_t wdt_get_timeout_val(void){	return WDT_TIMEOUT_MS; }
