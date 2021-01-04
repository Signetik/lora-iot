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
#include <logging/log.h>
#include "led_task.h"
#include "bt_task.h"
#include "lora_task.h"
#include "uart_task.h"
#include "vars.h"
#include "version.h"

// What	this version should	be:
const char *fw_rev = "+";
const char *fw_tag = "v0.8.0+";
const char *fw_branch =	"feature/bt_task";

LOG_MODULE_REGISTER(main,	CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

void main(void)
{
	
	vars_init();

	// Get Firmware	Version	string info	provided by	GIT	at compile time.
	// Compare it to hard-coded	intended version info defined above.
	strncpy(var_firmware.data, GIT_TAG,	var_firmware.size);
		
	if ((strncmp(GIT_REV, fw_rev, strlen(GIT_REV)))	||
		(strncmp(GIT_TAG, fw_tag ,strlen(GIT_TAG)))	||
		(strncmp(GIT_BRANCH, fw_branch,	strlen(GIT_BRANCH))))
	{
		LOG_ERR("Firmware version error!\n Version:	%s %s %s\n Expected: %s	%s %s\n", GIT_REV, GIT_TAG,	GIT_BRANCH,	fw_rev,	fw_tag,	fw_branch );
	}
		
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


