#include <zephyr.h>
#include <string.h>
#include <logging/log.h>
#include "timing_profile.h"
#include "relay_control.h"
#include "veridart_main_task.h"
#include "utility.h"


struct k_sem profile_mutex;
struct k_sem execution_start_semaphore;
struct k_sem cancel_operation_semaphore;
struct k_sem timer_callback_semaphore;

static timingprofile_t current_profile;
static uint16_t completed_oper_iterations;
static uint8_t current_operation_status;

LOG_MODULE_REGISTER(timingprofile, CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

void timing_profile_set(timingprofile_t* profile)
{	
	k_sem_take(&profile_mutex, K_FOREVER);
        memcpy(&current_profile, profile, sizeof(timingprofile_t));
    k_sem_give(&profile_mutex);
	
	LOG_INF("Set timing profile to: pre_delay_sec:%d, on_msec:%u, number_of_sprays:%d, off_sec:%d", profile->pre_delay_sec, profile->on_msec, profile->number_of_sprays, profile->off_sec);
}

void timing_profile_get(timingprofile_t* profile)
{	
	k_sem_take(&profile_mutex, K_FOREVER);
		memcpy(profile, &current_profile, sizeof(timingprofile_t));
	k_sem_give(&profile_mutex);
}

void timing_profile_get_operation_data(operation_data_t* op_data)
{
    uint8_t active = relay_is_active();

    k_sem_take(&profile_mutex, K_FOREVER);
        op_data->relay_active = active;
        op_data->completed_iterations = completed_oper_iterations;
    k_sem_give(&profile_mutex);
}

uint8_t operation_status_get()
{
	uint8_t status;
	
	k_sem_take(&profile_mutex, K_FOREVER);
		status = current_operation_status;
	k_sem_give(&profile_mutex);
	
	return status;
}

static void operation_status_set(uint8_t status)
{
	k_sem_take(&profile_mutex, K_FOREVER);
		current_operation_status = status;
	k_sem_give(&profile_mutex);
}

static void timing_profile_execute()
{
    uint32_t delay_iterations;
	uint32_t delay_remainder_ms;
	timingprofile_t temp_profile;
	
	operation_status_set(success);

    k_sem_take(&profile_mutex, K_FOREVER);
        completed_oper_iterations = 0;
        temp_profile = current_profile;
    k_sem_give(&profile_mutex);
	
	set_device_state(device_start_delay);
	
	//Perform the pre_delay_sec delay. Delay for X iterations of 1 second each.
	delay_iterations = temp_profile.pre_delay_sec;
	
	for (int i = 0; i < delay_iterations; i++)
	{
		if (k_sem_take(&cancel_operation_semaphore, K_NO_WAIT))
		{
			operation_status_set(canceled);
			return;
		}
		
		//sleep for 1 second
		k_sleep(K_MSEC(1000));
	}
    
    for (int j = 0; j < temp_profile.number_of_sprays; j++)
    {		
		relay_on();
		set_device_state(device_active_start);
		
		//Perform the on_msec delay. Delay for X iterations of 1 second each, plus remainder
		delay_iterations = temp_profile.on_msec / 1000;
		delay_remainder_ms = temp_profile.on_msec % 1000;
		
		for (int k = 0; k < delay_iterations; k++)
		{
			if (k_sem_take(&cancel_operation_semaphore, K_NO_WAIT))
			{
				relay_off();
				operation_status_set(canceled);
				return;
			}
				
			//sleep for 1 second
			k_sleep(K_MSEC(1000));
		}
		
		if (delay_remainder_ms > 0)
		{
			k_sleep(K_MSEC(delay_remainder_ms));
		}
		
        relay_off();
		set_device_state(device_active_end);
		
		//Perform the off_sec delay. Delay for X iterations of 1 second each.
		delay_iterations = temp_profile.off_sec;
		
		for (int z = 0; z < delay_iterations; z++)
		{
			if (k_sem_take(&cancel_operation_semaphore, K_NO_WAIT))
			{
				operation_status_set(canceled);
				return;
			}
					
			//sleep for 1 second
			k_sleep(K_MSEC(1000));
		}

        k_sem_take(&profile_mutex, K_FOREVER);
            completed_oper_iterations++;
        k_sem_give(&profile_mutex); 
    }
}

void timing_profile_task(void* p)
{
	LOG_INF("Timing profile task started");
	
	k_sem_init(&profile_mutex, 0, 1);
	k_sem_init(&execution_start_semaphore, 0, 1);
	k_sem_init(&cancel_operation_semaphore, 0, 1);
	k_sem_init(&timer_callback_semaphore, 0, 1);
	
	while(1)
	{
		k_sem_take(&execution_start_semaphore, K_FOREVER);
		 
		LOG_INF("Timing profile execution started!");
		 
		timing_profile_execute();
		
		uint8_t status = operation_status_get();
				 
	    if (status == success)
		{
			set_device_state(device_operation_complete);
		}
		else
		{
			set_device_state(device_operation_stopped);
		}
		
	}
}





