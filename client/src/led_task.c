/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

/*
 * Module Includes.
 */

#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <drivers/gpio.h>
#include <sys/base64.h>

#include <logging/log.h>

#include "signetik.h"
//#include "wdt_task.h"
#include "led_task.h"
#include "vars.h"

/*
 * Extract devicetree configuration.
 */

#define	DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE,	okay),
		 "No default LoRa radio	specified in DT");
#define	DEFAULT_RADIO DT_LABEL(DEFAULT_RADIO_NODE)

#define	LEDEN_NODE DT_ALIAS(leden)
#define	LEDR_NODE DT_ALIAS(ledr)
#define	LEDG_NODE DT_ALIAS(ledg)
#define	LEDB_NODE DT_ALIAS(ledb)

#if	DT_NODE_HAS_STATUS(LEDEN_NODE, okay)
#define	LEDEN_LABEL	DT_GPIO_LABEL(LEDEN_NODE, gpios)
#define	LEDEN_PIN	DT_GPIO_PIN(LEDEN_NODE,	gpios)
#define	LEDEN_FLAGS	DT_GPIO_FLAGS(LEDEN_NODE, gpios)
#endif
#if	DT_NODE_HAS_STATUS(LEDR_NODE, okay)
#define	LEDR_LABEL	DT_GPIO_LABEL(LEDR_NODE, gpios)
#define	LEDR_PIN	DT_GPIO_PIN(LEDR_NODE, gpios)
#define	LEDR_FLAGS	DT_GPIO_FLAGS(LEDR_NODE, gpios)
#endif
#if	DT_NODE_HAS_STATUS(LEDG_NODE, okay)
#define	LEDG_LABEL	DT_GPIO_LABEL(LEDG_NODE, gpios)
#define	LEDG_PIN	DT_GPIO_PIN(LEDG_NODE, gpios)
#define	LEDG_FLAGS	DT_GPIO_FLAGS(LEDG_NODE, gpios)
#endif
#if	DT_NODE_HAS_STATUS(LEDB_NODE, okay)
#define	LEDB_LABEL	DT_GPIO_LABEL(LEDB_NODE, gpios)
#define	LEDB_PIN	DT_GPIO_PIN(LEDB_NODE, gpios)
#define	LEDB_FLAGS	DT_GPIO_FLAGS(LEDB_NODE, gpios)
#endif


LOG_MODULE_REGISTER(lora, CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

/*
 * Module Defines
 */

/*
 * Module Variables.
 */

static struct device *led_en = NULL;
static struct device *led_r	= NULL;
static struct device *led_g	= NULL;
static struct device *led_b	= NULL;

K_MSGQ_DEFINE(led_msgq,	sizeof(led_msg_t), 5, 4);

/*
 * Local Functions
 */

/*
 * Public Functions
 */
void led_thread(void *p1, void *p2,	void *p3)
{

	static led_msg_t led_msg;

	led_r =	device_get_binding(LEDR_LABEL);
	gpio_pin_configure(led_r, LEDR_PIN,	GPIO_OUTPUT_INACTIVE | LEDR_FLAGS);
	led_g =	device_get_binding(LEDG_LABEL);
	gpio_pin_configure(led_g, LEDG_PIN,	GPIO_OUTPUT_INACTIVE | LEDG_FLAGS);
	led_b =	device_get_binding(LEDB_LABEL);
	gpio_pin_configure(led_b, LEDB_PIN,	GPIO_OUTPUT_INACTIVE | LEDB_FLAGS);
	led_en = device_get_binding(LEDEN_LABEL);
	gpio_pin_configure(led_en, LEDEN_PIN, GPIO_OUTPUT_INACTIVE | LEDEN_FLAGS);

	while (1)
	{
		LOG_DBG("LoRa Thread loop...");
		gpio_pin_set(led_r,	LEDR_PIN,	led_msg.red);
		gpio_pin_set(led_g,	LEDG_PIN,	led_msg.green);
		gpio_pin_set(led_b,	LEDB_PIN,	led_msg.blue);
		gpio_pin_set(led_en, LEDEN_PIN,	led_msg.enable);

		k_msgq_get(&led_msgq, &led_msg,	K_FOREVER);
	}
}


///	Create Led thread/task.
K_THREAD_STACK_DEFINE(led_stack_area, LED_STACKSIZE);
struct k_thread	led_thread_data;

///	Start Led thread.
void led_thread_start(void)
{
	/* k_tid_t my_tid =	*/
	k_thread_create(&led_thread_data, led_stack_area,
		K_THREAD_STACK_SIZEOF(led_stack_area),
		led_thread,
		NULL, NULL,	NULL,
		LED_PRIORITY, 0, K_NO_WAIT);
}

