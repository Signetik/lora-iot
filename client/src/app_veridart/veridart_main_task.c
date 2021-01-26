//============================================================================//
//                                                                            //
//        .d8888b.  d8b                            888    d8b 888             //
//       d88P  Y88b Y8P                            888    Y8P 888             //
//       Y88b.                                     888        888             //
//        "Y888b.   888  .d88b.  88888b.   .d88b.  888888 888 888  888        //
//           "Y88b. 888 d88P"88b 888 "88b d8P  Y8b 888    888 888 .88P        //
//             "888 888 888  888 888  888 88888888 888    888 888888K         //
//       Y88b  d88P 888 Y88b 888 888  888 Y8b.     Y88b.  888 888 "88b        //
//        "Y8888P"  888  "Y88888 888  888  "Y8888   "Y888 888 888  888        //
//                           888                                              //
//                      Y8b d88P                                              //
//                       "Y88P"                                Signetik, LLC  //
//                                                           www.signetik.com //
//----------------------------------------------------------------------------//
//           Copyright © 2020 Signetik, LLC -- All Rights Reserved            //
//   Signetik Confidential Proprietary Information -- Disclosure Prohibited    //
//----------------------------------------------------------------------------//
// Project   : SafeTraces veriDART                                            //
// Filename  :                                                                //
// Author(s) :                                                                //
// Created   :                                                                //
// Purpose   :                                                                //
//============================================================================//
// NOTE: Signetik will transfer ownership to SafeTraces on final delivery.

/*
 * veridart_main_task.c
 *
 * Created: 8/19/2020 10:03:25 AM
 *  Author: Remy Patterson
 */

#include <logging/log.h>
#include <random/rand32.h>

#include "veridart_main_task.h"
#include "veridart-messages.h"
#include "../signetik.h"
#include "main.h"
#include "utility.h"

LOG_MODULE_REGISTER(veridarttask, CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

K_SEM_DEFINE(veridart_main_mutex, 0, 1);

uint8_t current_device_state;
uint8_t device_tx_quiet = 0;

void set_device_state(uint8_t state)
{
	k_sem_take(&veridart_main_mutex, K_FOREVER);
		current_device_state = state;
		if(state == device_operation_complete) {
			device_tx_quiet = 0;
		}
		if((state == device_start_delay) || 
		(state == device_active_start) ||
		(state == device_active_end)) {
			// In case Ack was lost and we kept Tx-ing... stop now while operation is underway.
			device_tx_quiet = 1;
		}
	k_sem_give(&veridart_main_mutex);
	
	LOG_DBG("device state set to %d", state);

	//veridart_queue_uplink(uplink_statechange);
}

uint8_t get_device_state()
{
	uint8_t state;
	
	k_sem_take(&veridart_main_mutex, K_FOREVER);
		state = current_device_state;
	k_sem_give(&veridart_main_mutex);

	return state;
}

void set_device_tx_quiet(uint8_t state)
{
	k_sem_take(&veridart_main_mutex, K_FOREVER);
		device_tx_quiet = state;
	k_sem_give(&veridart_main_mutex);
	
	LOG_DBG("device Tx quiet set to %d", state);

}

uint8_t get_device_tx_quiet()
{
	uint8_t state;
	
	k_sem_take(&veridart_main_mutex, K_FOREVER);
		state = device_tx_quiet;
	k_sem_give(&veridart_main_mutex);

	return state;
}

//Returns a random delay between 0ms and 2000ms
static uint32_t get_random_delay()
{
	//Note: rand()returns a random number, max 65536
	uint32_t random_number = (sys_rand32_get() % (2000 + 1));
	LOG_DBG("random delay: %d ms", random_number);
	
	return random_number;
}


//Device has just booted, waiting to get time from server
static void execute_state_device_booted()
{
	k_sleep(K_MSEC(10000 + get_random_delay())); //wait a bit
	veridart_queue_uplink(uplink_status);
}

//Device has received time from server, waiting to get timing profile
static void execute_state_device_timesetup()
{
	k_sleep(K_MSEC(5000 + get_random_delay())); //wait a bit
	veridart_queue_uplink(uplink_status);
}

//Device has received timing profile, waiting for 'go' from server
static void execute_state_device_obtained_timing_profile()
{
	k_sleep(K_MSEC(5000 + get_random_delay())); //wait a bit
	veridart_queue_uplink(uplink_status);
}

//Device has received go, waiting for start time
static void execute_state_device_obtained_go()
{
	k_sleep(K_MSEC(5000 + get_random_delay())); //wait a bit 
	veridart_queue_uplink(uplink_status);
}

//Device is waiting for a duration of "pre_delay_sec"
static void execute_state_device_start_delay()
{
	k_sleep(K_MSEC(30000 + get_random_delay())); //wait a bit
	//veridart_queue_uplink(uplink_status);
}

//Device is executing the timing profile, in 'device_active_start' or 'device_active_end' state
static void execute_state_device_active()
{
	k_sleep(K_MSEC(20000 + get_random_delay())); //wait a bit
	//veridart_queue_uplink(uplink_status);
}

//Device has finished running operation, waiting for next instruction from server
static void execute_state_device_operation_complete()
{
	k_sleep(K_MSEC(10000 + get_random_delay())); //wait a bit
	veridart_queue_uplink(uplink_status);
}

//Device operation was stopped, waiting for next instruction from server
static void execute_state_device_operation_stopped()
{	
	k_sleep(K_MSEC(20000 + get_random_delay())); //wait a bit
	veridart_queue_uplink(uplink_status);
}


void veridart_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("Veridart main task started");
	
#ifdef STEVE
	LOG_INF("ResetCause: 0x%02X", system_get_reset_cause());
#endif
	
	set_device_state(device_booted); //Device will start in state device_booted
	
	while(1)
	{		
		switch(current_device_state)
		{
			case device_booted :
				execute_state_device_booted();
				break;
			case device_timesetup :
				execute_state_device_timesetup();
				break;
			case device_obtained_timing_profile:
				execute_state_device_obtained_timing_profile();
				break;
			case device_obtained_go:
				execute_state_device_obtained_go();
				break;
			case device_start_delay :
				execute_state_device_start_delay();
				break;
			case device_active_start :
			case device_active_end :
				execute_state_device_active();
				break;
			case device_operation_complete :
				execute_state_device_operation_complete();
				break;
			case device_operation_stopped :
				execute_state_device_operation_stopped();
				break;
			default :
				LOG_ERR("Error: device state invalid");
				break;
		}
	}	
};

///	Create LoRa	thread/task.
K_THREAD_STACK_DEFINE(veridart_stack_area, VERIDART_STACKSIZE);
struct k_thread	veridart_thread_data;

///	Start Veridart thread.
void veridart_thread_start(void)
{
	/* k_tid_t my_tid =	*/
	k_thread_create(&veridart_thread_data, veridart_stack_area,
		K_THREAD_STACK_SIZEOF(veridart_stack_area),
		veridart_thread,
		NULL, NULL,	NULL,
		VERIDART_PRIORITY, 0, K_NO_WAIT);
}

void custom_app_start(void)
{
	veridart_thread_start();
}
