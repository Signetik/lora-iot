/*
 * Copyright (c) 2016 Intel	Corporation
 *
 * SPDX-License-Identifier:	Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include "led_task.h"
#include "bt_task.h"
#include "lora_task.h"
#include "uart_task.h"
#include "wdt_task.h"
#include "vars.h"
#include "sigversion.h"

#if	!defined(CONFIG_SIGNETIK_APP_NONE)
void custom_app_start(void);
#endif

void main(void)
{
	
	vars_init();

	// Get Firmware	Version	string.
	strncpy(var_firmware.data, GIT_TAG,	var_firmware.size);
		
	// Start RTOS task threads.
	wdt_thread_start();	// @todo Should	this start before other	threads??
	uart_thread_start();
	led_thread_start();
	bt_thread_start();
	lora_thread_start();
#if	!defined(CONFIG_SIGNETIK_APP_NONE)
	custom_app_start();
#endif

	while(1)
	{
		k_sleep(K_MSEC(1000));
	}
}


