/*============================================================================*
 *         Copyright © 2019-2021 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                          SPDX-License-Identifier: Sigentik *
 *                                                                            *
 * Customer may modify, compile, assemble and convert this source code into   *
 * binary object or executable code for use on Signetk products purchased     *
 * from Signetik or its distributors.                                         *
 *                                                                            *
 * Customer may incorporate or embed an binary executable version of the      *
 * software into the Customer’s product(s), which incorporate a Signetik      *
 * product purchased from Signetik or its distributors. Customer may          *
 * manufacture, brand and distribute such Customer’s product(s) worldwide to  *
 * its End-Users.                                                             *
 *                                                                            *
 * This agreement must be formalized with Signetik before Customer enters     *
 * production and/or distributes products to Customer's End-Users             *
 *============================================================================*/

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


