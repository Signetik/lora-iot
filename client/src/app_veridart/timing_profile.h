#ifndef __TIMING_PROFILE_H
#define __TIMING_PROFILE_H

#include <stdint.h>
#include "veridart-messages.h"

extern struct k_sem execution_start_semaphore;
extern struct k_sem  cancel_operation_semaphore;
extern struct k_timer timing_profile_timer;


void timing_profile_set(timingprofile_t* profile);
void timing_profile_get(timingprofile_t* profile);
void timing_profile_get_operation_data(operation_data_t* op_data);
uint8_t operation_status_get(void);
void timing_profile_task(void* p);
void vTimerCallback( struct k_timer xTimer );

#endif /* __TIMING_PROFILE_H */
