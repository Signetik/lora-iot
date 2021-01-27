#ifndef __TIMING_PROFILE_H
#define __TIMING_PROFILE_H

#include <stdint.h>
#include "veridart-messages.h"

extern struct k_sem execution_start_semaphore;
extern struct k_sem  cancel_operation_semaphore;
extern struct k_timer timing_profile_timer;

#define TIMING_PROFILE_STACKSIZE   512
#define TIMING_PROFILE_PRIORITY	   8

void timing_profile_set(timingprofile_t* profile);
void timing_profile_get(timingprofile_t* profile);
void timing_profile_get_operation_data(operation_data_t* op_data);
uint8_t operation_status_get(void);

void timing_profile_thread_start(void);

#endif /* __TIMING_PROFILE_H */
