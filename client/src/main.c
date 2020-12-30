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
#include "vars.h"


void main(void)
{
	
	vars_init();

	
	// Start RTOS task threads.
	uart_thread_start();
	led_thread_start();
	bt_thread_start();
	lora_thread_start();
//	wdt_thread_start();	// @todo Should	this start before other	threads??

	while(1)
	{
		k_sleep(K_MSEC(1000));
	}
}


