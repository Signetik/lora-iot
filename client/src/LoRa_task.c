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
#include <lorawan/lorawan.h>
#include <device.h>
#include <zephyr.h>

#include <drivers/gpio.h>
#include <drivers/lora.h>
#include <lorawan/lorawan.h>

#include <sys/base64.h>

#include <logging/log.h>

#include "signetik.h"
//#include "wdt_task.h"
#include "lora_task.h"
#include "uart_task.h"
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

/*
 * Module Defines
 */
#define	LORA_TX_BUF_SIZE 255

#define	APP_COAP_SEND_INTERVAL_MS K_MSEC(5000)
#define	APP_COAP_MAX_MSG_LEN (2048 + 16)
#define	APP_COAP_VERSION 1

#define	MAX_TX_DATA_LEN	12
#define	MAX_RX_DATA_LEN	255
//#define	TX_CW

/*
 * Module Variables.
 */
uint8_t	thread_id;

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

	if (strcmp(var_lora_auth.data, "abp") == 0)
	{
	//	Authentication by personalization
		join_cfg->mode = LORAWAN_ACT_ABP;
		join_cfg->dev_eui =	var_lora_dev_eui.data;			// var_lora_dev_eui.data;
		join_cfg->abp.dev_addr = var_lora_dev_addr;			// device address
		join_cfg->abp.app_skey = var_lora_app_skey.data;		// var_lora_app_skey.data;
		join_cfg->abp.nwk_skey = var_lora_nwk_skey.data;		// var_lora_nwk_skey.data;
		join_cfg->abp.app_eui =	var_lora_dev_eui.data;			// var_lora_app_eui.data;
	}
	else if	(strcmp(var_lora_auth.data,	"otaa")	== 0)
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

void lorawan_rx_data(uint8_t *buffer, int sz)
{
	uint8_t	obuffer[64];
	size_t obuffer_len = 64;

	if (sz > 0)	{
		base64_encode(obuffer, obuffer_len,	&obuffer_len, buffer, sz);
		
		k_sem_take(&sem_rx_cb, K_FOREVER);
		uart_send("+notify,lora:rx,base64:", 0);
		uart_send(obuffer, obuffer_len);
		uart_send("\r\n", 0);
		k_sem_give(&sem_rx_cb);
	}
}

#define	FORCE_GPIO_1_7_HIGH	1

/*
 * Public Functions
 */
void lora_thread(void *p1, void	*p2, void *p3)
{
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
//		wdt_feed_watchdog(thread_id);

		if (!var_connected && var_enabled) 
		{
			static uint16_t	channels[5];
			channels[0]	= channels[1] =	channels[2]	= channels[3] =	0;
			channels[0]	= 0x0f00;
			channels[4]	= 0xff;

			if (0 == lorawan_start())
			{
				uart_send("+notify,lora:join,status:start\r\n",	0);
			}
			lorawan_set_class(LORAWAN_CLASS_C);
			lorawan_enable_adr(false);
			lorawan_set_datarate(LORAWAN_DR_1);

			lorawan_set_channelmask(channels);

			lorawan_set_rxcb(lorawan_rx_data);

			if (0 != lora_configure(&lw_config))
			{
				LOG_ERR("Invalid LoRa configuration.");
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
#if(0)
			lorawan_send(1,	txData,	MAX_TX_DATA_LEN, 0 /*LORAWAN_MSG_CONFIRMED*/);
			k_sleep(K_MSEC(5000));
#endif
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
	uint8_t	buffer[LORA_TX_BUF_SIZE];
	//uint32_t data_len;
	size_t blen	= sizeof(buffer);
	
	const struct device	*lora_dev;
	lora_dev = device_get_binding(DEFAULT_RADIO);
	if (!lora_dev) 
	{
		LOG_ERR("%s	Device not found", DEFAULT_RADIO);
		return ret;
	}
	else
	{
		//data_len = snprintf(buffer,	sizeof(buffer),	"%s,%s", key, value);
		ret	= base64_decode(buffer,	blen, &blen, &value[1],	strlen(value)-2);
		if ((ret ==	0) && (blen	<= LORA_TX_BUF_SIZE) &&	(blen >	0))
		{	
			k_sem_take(&sem_lora_push, K_FOREVER);
			//ret	= lora_send(lora_dev, buffer, blen);
			ret	= lorawan_send(1, buffer, blen,	0 /*LORAWAN_MSG_CONFIRMED*/);	
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

