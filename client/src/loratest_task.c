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

#include <zephyr.h>
#include <logging/log.h>
#include "LoRa_task.h"

#define LORATEST_STACKSIZE   512
#define LORATEST_PRIORITY	   8

LOG_MODULE_REGISTER(loratesttask, CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

void loratest_thread(void *p1, void *p2, void *p3)
{
	struct lora_tx_message msg;

	LOG_INF("Lora test task started");
	
	var_enabled = true;
	
	while(1)
	{
        k_sleep(K_SECONDS(10));

        msg.message[0] = 'T';
        msg.message[1] = 'E';
        msg.message[2] = 'S';
        msg.message[3] = 'T';
	
	    msg.length=4;
	
	    k_msgq_put(&lora_tx_queue, &msg, K_NO_WAIT);
    }
}

///	Create LoRa test thread/task.
K_THREAD_STACK_DEFINE(loratest_stack_area, LORATEST_STACKSIZE);
struct k_thread	loratest_thread_data;

///	Start LoRa test thread.
void loratest_thread_start(void)
{
	/* k_tid_t my_tid =	*/
	k_thread_create(&loratest_thread_data, loratest_stack_area,
		K_THREAD_STACK_SIZEOF(loratest_stack_area),
		loratest_thread,
		NULL, NULL,	NULL,
		LORATEST_PRIORITY, 0, K_NO_WAIT);
}

void custom_app_start(void)
{
	loratest_thread_start();
}

void custom_app_rx(uint8_t *data, int sz)
{
}
