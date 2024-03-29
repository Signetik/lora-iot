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

/*
 * Module Includes.
 */

#include <stdio.h>
#include <string.h>
#include <lorawan/lorawan.h>
#include <device.h>
#include <zephyr.h>

#include <drivers/gpio.h>
#include <drivers/lora.h>
#include <lorawan/lorawan.h>

#include <sys/base64.h>

#include <logging/log.h>

#include "signetik.h"
#include "wdt_task.h"
#include "lora_task.h"
#include "uart_task.h"
#include "led_task.h"
#include "vars.h"

/*
 * Extract devicetree configuration.
 */

#define	DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE,	okay),
		 "No default LoRa radio	specified in DT");
#define	DEFAULT_RADIO DT_LABEL(DEFAULT_RADIO_NODE)


LOG_MODULE_REGISTER(loratask,	CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

K_SEM_DEFINE(sem_rx_cb,	0, 1);
K_SEM_DEFINE(sem_lora_push,	0, 1);
K_SEM_DEFINE(sem_tx_busy, 0, 1);

K_MSGQ_DEFINE(lora_tx_queue, sizeof(struct lora_tx_message), 4,	4);	/* 4 messages, 4 byte alignment	*/

/*
 * Module Defines
 */
#define	LORA_TX_BUF_SIZE 255

#define	MAX_TX_DATA_LEN	12
#define	MAX_RX_DATA_LEN	255
//#define	TX_CW

void custom_app_rx(uint8_t *buffer, int sz);

/*
 * Module Variables.
 */

struct lora_modem_config config	= 
{
	.frequency = 902300000,
	.bandwidth = BW_125_KHZ,
	.datarate	= SF_10,
	.preamble_len	= 8,
	.coding_rate = CR_4_5,
	.tx_power	= -9, //4;	-9dBm for testing w/o antenna
	.tx	=	true
};

char txData[MAX_TX_DATA_LEN] = {0x64, 0x01,	0x00, 0x00,	0x00, 0x0E,	0xE1, 0x39,	0x00, 0x00,	0x00, 0x00};
uint8_t	rxData[MAX_RX_DATA_LEN]	= {0};

/*
 * Local Functions
 */
static int lora_configure(struct lorawan_join_config *join_cfg)
{
	int	ret	= 0;

	// determine Class operation
	if (strcmp(strupr(var_lora_class.data),	"A") ==	0)
	{
		lorawan_set_class(LORAWAN_CLASS_A);
	}
	else if	(strcmp(strupr(var_lora_class.data), "C") == 0)
	{
		lorawan_set_class(LORAWAN_CLASS_C);
	}
	else
	{
		ret	= -1;
	}

	// Adaptive	data rate selection
	lorawan_enable_adr(var_lora_adr);

	// Data	rate selection	(Function ignores datarate if ADR is set.)
	lorawan_set_datarate(var_lora_datarate);

	// Channel mask	selection
	lorawan_set_channelmask((uint16_t*)&var_lora_chan_mask.data[0]);

	// RX delay setting
	lorawan_set_rxdelay(var_lora_rxdelay1, var_lora_rxdelay2);

	lorawan_set_datarate(var_lora_datarate);

	if (strcmp(strupr(var_lora_auth.data),	"ABP")	== 0)
	{
	//	Authentication by personalization
		join_cfg->mode = LORAWAN_ACT_ABP;
		join_cfg->dev_eui =	var_lora_dev_eui.data;			// var_lora_dev_eui.data;
		join_cfg->abp.dev_addr = *((uint32_t*)var_lora_dev_addr.data);			// device address
		join_cfg->abp.app_skey = var_lora_app_skey.data;		// var_lora_app_skey.data;
		join_cfg->abp.nwk_skey = var_lora_nwk_skey.data;		// var_lora_nwk_skey.data;
		join_cfg->abp.app_eui =	var_lora_dev_eui.data;			// var_lora_app_eui.data;
	}
	else if	(strcmp(strupr(var_lora_auth.data),	"OTAA")	== 0)
	{
	//	Over The Air Authentication
		join_cfg->mode = LORAWAN_ACT_OTAA;
		join_cfg->dev_eui =	var_lora_dev_eui.data;
		join_cfg->otaa.join_eui	= var_lora_app_eui.data;
		join_cfg->otaa.app_key = var_lora_app_key.data;
	}
	else
	{
	// Invalid option, clear the configuration structure
		memset(&join_cfg, 0x0, sizeof(join_cfg));
		ret	= -1;
	}
	return ret;
}

void display_base64_data(uint8_t *buffer, int sz);

void lorawan_tx_data(bool success, uint32_t	channel, uint8_t data_rate, uint8_t tx_power)
{
	char status_str[32];

	static led_msg_t led_msg = {
		.red = true,
		.green = false,
		.blue =	true,
		.enable	= false
	};

	k_msgq_put(&led_msgq, &led_msg,	K_MSEC(100));
	k_sem_give(&sem_tx_busy);

	snprintf(status_str, sizeof(status_str)-1, "chan:%d,dr:%d,pow:%d",	channel, data_rate, tx_power);

	k_sem_take(&sem_rx_cb, K_FOREVER);
	if (success) {
		uart_send("+notify,lora:tx,status:success,", 0);
		LOG_INF("+notify,lora:tx,status:success,");
	}
	else {
		uart_send("+notify,lora:tx,status:fail,", 0);
		LOG_INF("+notify,lora:tx,status:fail,");
	}
	uart_send(status_str, 0);
	uart_send("\r\n", 0);
	LOG_INF("%s", log_strdup(status_str));
	k_sem_give(&sem_rx_cb);
}

void lorawan_rx_data(uint8_t *buffer, int sz, uint8_t port, int16_t rssi, uint8_t data_rate)
{
	char status_str[32];

	led_msg_t led_msg = {
		.red = false,
		.green = true,
		.blue =	false,
		.enable	= true
	};

	k_msgq_put(&led_msgq, &led_msg,	K_MSEC(100));

	if (sz > 0)	{
#if	!defined(CONFIG_SIGNETIK_APP_NONE)
		custom_app_rx(buffer, sz);
#endif
		snprintf(status_str, sizeof(status_str)-1, ",port:%d,dr:%d,rssi:%d",	port, data_rate, rssi);

		k_sem_take(&sem_rx_cb, K_FOREVER);
		uart_send("+notify,lora:rx,base64:", 0);
		LOG_INF("+notify,lora:rx,base64:");
		display_base64_data(buffer, sz);
		uart_send(status_str, 0);
		uart_send("\r\n", 0);
		LOG_INF("%s", log_strdup(status_str));
		k_sem_give(&sem_rx_cb);
	}

	led_msg.enable = false;
	k_msgq_put(&led_msgq, &led_msg,	K_MSEC(100));
}

void display_base64_data(uint8_t *buffer, int sz)
{
	uint8_t	obuffer[64];
	size_t obuffer_len = 64;

	base64_encode(obuffer, obuffer_len,	&obuffer_len, buffer, sz);
	uart_send(obuffer, obuffer_len);
	obuffer[obuffer_len] = 0;
	LOG_INF("%s", log_strdup(obuffer));
}

#define	FORCE_GPIO_1_7_HIGH	1

static struct lora_tx_message msg;

/*
 * Public Functions
 */
void lora_thread(void *p1, void	*p2, void *p3)
{
	uint8_t	thread_id;
	int16_t	rssi;
	int8_t snr;
	int	len;
	struct lorawan_join_config lw_config;	/* loraWan configuration */
	int	ret;
	int	err;
	int	record_number =	0;
	char txData[MAX_TX_DATA_LEN] = {0x64, 0x01,	0x00, 0x00,	0x00, 0x0E,	0xE1, 0x39,	0x00, 0x00,	0x00, 0x00};
	uint8_t	rxData[MAX_RX_DATA_LEN]	= {0};

	const struct device	*lora_dev;
	const struct device	*dev1;

	lora_dev = device_get_binding(DEFAULT_RADIO);
	if (!lora_dev) 
	{
		LOG_ERR("%s	Device not found", DEFAULT_RADIO);
		return;
	}

	// Register	with WDT.
	thread_id =	wdt_register_thread();

	k_sem_give(&sem_rx_cb);
	k_sem_give(&sem_lora_push);

	// Register	with WDT.
//	thread_id =	wdt_register_thread();

#if	defined(FORCE_GPIO_1_7_HIGH)
	dev1 = device_get_binding("GPIO_1");
	gpio_pin_configure(dev1, 7,	GPIO_OUTPUT_ACTIVE);
	gpio_pin_set(dev1, 7,	1);
#endif
#if	(1)

	while (1)
	{
		LOG_DBG("LoRa Thread loop...");

		// Feed	WDT	(must use assigned thread ID).
		wdt_feed_watchdog(thread_id);

		if (!var_connected && var_enabled) 
		{
			if (0 == lorawan_start())
			{
				uart_send("+notify,lora:join,status:start\r\n",	0);
			}

			lorawan_set_txcb(lorawan_tx_data);
			lorawan_set_rxcb(lorawan_rx_data);
			
			ret	= lora_configure(&lw_config);
			if (0 != ret)
			{
				LOG_ERR("Invalid LoRa configuration.");
				printk("(ret %d)\n", ret);
				uart_send("+notify,lora:join,status:rejected\r\n", 0);
			}
			else
			{
				uart_send("+notify,lora:join,status:send\r\n", 0);
				switch (lorawan_join(&lw_config))
				{
				case 0:
					uart_send("+notify,lora:join,status:complete\r\n", 0);
					var_connected =	true;
					break;

				case -ETIMEDOUT:
					uart_send("+notify,lora:join,status:timeout\r\n", 0);
					break;

				case -ENOTCONN:
				case -ECONNREFUSED:
				default:
					uart_send("+notify,lora:join,status:rejected\r\n", 0);
					break;
				}
			}
			k_sleep(K_MSEC(1000));
		}
		else if	(var_enabled)
		{		/* Block until data	arrives	or 5 seconds	passes */
			k_sleep(K_MSEC(1000));
			if (k_msgq_get(&lora_tx_queue, &msg, K_NO_WAIT)	== 0) {
				led_msg_t led_msg = {
					.red = true,
					.green = false,
					.blue =	true,
					.enable	= true
				};

				k_sem_take(&sem_tx_busy, K_SECONDS(10));
				k_msgq_put(&led_msgq, &led_msg,	K_MSEC(100));

				LOG_INF("Send Lora Packet");
				uart_send("+notify,lora:tx,status:send,base64:", 0);
				display_base64_data(msg.message, msg.length);
				uart_send("\r\n", 0);
				ret = lorawan_send(1, msg.message, msg.length, 0 /*LORAWAN_MSG_CONFIRMED*/);
				if (ret != 0) {
					char status_str[16];

					snprintf(status_str, sizeof(status_str)-1, "err:%d", ret);

					uart_send("+notify,lora:tx,status:fail,", 0);
					uart_send(status_str, 0);
					uart_send("\r\n", 0);

					led_msg.blue = false;
					k_msgq_put(&led_msgq, &led_msg, K_MSEC(100));
					k_sleep(K_MSEC(1000));
					led_msg.enable = false;
					k_msgq_put(&led_msgq, &led_msg, K_MSEC(100));
				}
			}
#if(0)			
			len	= lora_recv(lora_dev, rxData,	MAX_RX_DATA_LEN, K_MSEC(5000), &rssi, &snr);

			if (len	< 0)
			{
				LOG_ERR("LoRa receive failed");
			}

			if (len	> 0) 
			{
				LOG_INF("Received data:	%s (RSSI:%ddBm,	SNR:%ddBm)",
				log_strdup(rxData),	rssi,	snr);
				uart_send("+notify,lora:rx,base64:\r\n", 0);
			}
#endif
		}
		else
		{
			// prohibit	sending	if lora	access now disabled
			var_connected =	false;
			k_sleep(K_MSEC(1000));
		}
	}
#elif(1)
	static uint16_t	channels[5];
	channels[0]	= channels[1] =	channels[2]	= channels[3] =	0;
	channels[0]	= 0x0f00;
	channels[4]	= 0xff;

	lorawan_start();
	lorawan_set_class(LORAWAN_CLASS_C);
	lorawan_enable_adr(false);
	lorawan_set_datarate(LORAWAN_DR_1);
	lorawan_set_channelmask(channels);

	const struct lorawan_join_config lw_config2	= {
		.abp = {
			0x26022001,	/* devid */
			app_skey,
			nwk_skey,
			app_eui
		},
		.dev_eui = dev_eui,
		.mode =	LORAWAN_ACT_ABP
	};
	lorawan_join(&lw_config2);
	while (1) {
		lorawan_send(1,	txData,	MAX_TX_DATA_LEN, 0 /*LORAWAN_MSG_CONFIRMED*/);
		k_sleep(K_MSEC(5000));
	}
#elif	defined(TX_CW)
	ret	= lora_config(lora_dev,	&config);
	if (ret	< 0) {
		LOG_ERR("LoRa config failed");
		return;
	}
	// Start continuous	wave transmission function expires after 65535 seconds
	lora_test_cw(lora_dev, config.frequency, config.tx_power,	65535);

	while(1) {
		LOG_INF("Continuous	Wave Tx	active.");
		k_sleep(K_MSEC(1000));
	}

#elif 0
	char txData[MAX_TX_DATA_LEN] = {'h', 'e', 'l', 'l',	'o', 'w', 'o', 'r',	'l', 'd'};
	uint8_t	rxData[MAX_RX_DATA_LEN]	= {0};
	while (1) {
		ret	= lora_config(lora_dev,	&config);
		ret	= lora_send(lora_dev, txData,	MAX_TX_DATA_LEN);
		if (ret	< 0) {
			LOG_ERR("LoRa send failed");
		}
		else {
			LOG_INF("Data sent!");
		}

		/* Block until data	arrives	or 5 seconds	passes */
		int16_t	rssi;
		int8_t snr;
		int	len	= lora_recv(lora_dev, rxData,	MAX_RX_DATA_LEN, K_MSEC(5000),
				&rssi, &snr);

		if (len	< 0) {
			LOG_ERR("LoRa receive failed");
		}

		if (len	> 0) {
			LOG_INF("Received data:	%s (RSSI:%ddBm,	SNR:%ddBm)",
			log_strdup(rxData),	rssi,	snr);

		}
	}
#endif
}

int	lora_push(char *key, char *value)
{
	int	ret	= -1;
	struct lora_tx_message msg;
	size_t msglen =	sizeof(msg.message);
	const struct device	*lora_dev;

	lora_dev = device_get_binding(DEFAULT_RADIO);
	if (!lora_dev) 
	{
		LOG_ERR("%s	Device not found", DEFAULT_RADIO);
		return ret;
	}
	else
	{
		ret	= base64_decode(msg.message, msglen, &msglen, &value[1], strlen(value)-2);
		if ((ret ==	0) && (msglen <= sizeof(msg.message)) && (msglen >	0))
		{	
			k_sem_take(&sem_lora_push, K_FOREVER);
			msg.length = msglen;

			ret	= k_msgq_put(&lora_tx_queue, &msg, K_NO_WAIT);
			if (0 != ret)
			{
				LOG_ERR("Lora Tx queue full\n");
			}
			k_sem_give(&sem_lora_push);
		}
	}
	return ret;
}

///	Create LoRa	thread/task.
K_THREAD_STACK_DEFINE(lora_stack_area, LORA_STACKSIZE);
struct k_thread	lora_thread_data;

///	Start LoRa thread.
void lora_thread_start(void)
{
	/* k_tid_t my_tid =	*/
	k_thread_create(&lora_thread_data, lora_stack_area,
		K_THREAD_STACK_SIZEOF(lora_stack_area),
		lora_thread,
		NULL, NULL,	NULL,
		LORA_PRIORITY, 0, K_NO_WAIT);
}

