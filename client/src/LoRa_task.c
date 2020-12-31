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
#include <net/coap.h>
#include <net/sntp.h>
#include <net/socket.h>
#include <fcntl.h>
//#include <lte_lc.h>
//#include <at_cmd.h>
#include <drivers/gpio.h>
#include <drivers/lora.h>
#include <lorawan/lorawan.h>

//#include <dfu/mcuboot.h>
//#include <dfu/dfu_target.h>
//#include <sys/base64.h>

#include <logging/log.h>

#include "signetik.h"
//#include "wdt_task.h"
#include "lora_task.h"
//#include "lwm2m_handler.h"
//#include "transports/signetik_coap.h"
//#include "transports/signetik_mqtt.h"
//#include "fota_download.h"
#include "vars.h"
//#include "cell_packet.h"
//#include "coap_cbor_device.h"

/*
 * Extract devicetree configuration.
 */

#define	DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE,	okay),
		 "No default LoRa radio	specified in DT");
#define	DEFAULT_RADIO DT_LABEL(DEFAULT_RADIO_NODE)


LOG_MODULE_REGISTER(loratask, CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

/*
 * Module Defines
 */

#define	APP_COAP_SEND_INTERVAL_MS K_MSEC(5000)
#define	APP_COAP_MAX_MSG_LEN (2048 + 16)
#define	APP_COAP_VERSION 1

#define	MAX_TX_DATA_LEN	12
#define	MAX_RX_DATA_LEN	255
#define	TX_CW

/*
 * Module Variables.
 */
uint8_t	thread_id;
struct lora_modem_config config;
/*
 * Local Functions
 */

/*
 * Public Functions
 */
void lora_thread(void *p1, void	*p2, void *p3)
{
	int	ret;
	int	err;
	int	record_number =	0;
	char txData[MAX_TX_DATA_LEN] = {0x64, 0x01, 0x00, 0x00, 0x00, 0x0E, 0xE1, 0x39, 0x00, 0x00, 0x00, 0x00};
	uint8_t	rxData[MAX_RX_DATA_LEN]	= {0};

	const struct device	*lora_dev;
	const struct device *dev1;

	uint8_t app_eui[] = {0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x03, 0x31, 0xC9};
	uint8_t nwk_skey[] = {0x0B, 0x30, 0x52, 0x51, 0xA6, 0x0C, 0x52, 0x11, 0x72, 0x32, 0x85, 0xD1, 0xFB, 0x2E, 0xF8, 0x39};
	uint8_t app_skey[] = { 0x6F, 0x7B, 0x80, 0xF7, 0xE4, 0xD0, 0xB9, 0xE5, 0x1F, 0xE9, 0xF8, 0x97, 0x64, 0x15, 0xBD, 0xD7 };
	uint8_t dev_eui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0x4c };

	// Register	with WDT.
//	thread_id =	wdt_register_thread();
//
	//dev1 = device_get_binding("GPIO_1");
	//gpio_pin_configure(dev1, 7,	GPIO_OUTPUT_ACTIVE);
	//gpio_pin_set(dev1, 7,	1);

#if(1)
	lorawan_start();
	lorawan_set_class(LORAWAN_CLASS_C);
	lorawan_enable_adr(false);
	lorawan_set_datarate(LORAWAN_DR_1);

	const struct lorawan_join_config lw_config = {
		.abp = {
			0x26029100, /* devid */
			app_skey,
			nwk_skey,
			app_eui
		},
		.dev_eui = dev_eui,
		.mode = LORAWAN_ACT_ABP
	};
	lorawan_join(&lw_config);
	while (1) {
		lorawan_send(1, txData, MAX_TX_DATA_LEN, 0 /*LORAWAN_MSG_CONFIRMED*/);
		k_sleep(K_MSEC(5000));
	}
#endif

	lora_dev = device_get_binding(DEFAULT_RADIO);
	if (!lora_dev) {
		LOG_ERR("%s	Device not found", DEFAULT_RADIO);
		return;
	}

	config.frequency = 902300000;
	config.bandwidth = BW_125_KHZ;
	config.datarate	= SF_10;
	config.preamble_len	= 8;
	config.coding_rate = CR_4_5;
	config.tx_power	= -9; //4;	-9dBm for testing w/o antenna
	config.tx =	true;

	ret	= lora_config(lora_dev,	&config);
	if (ret	< 0) {
		LOG_ERR("LoRa config failed");
		return;
	}
#if	defined(TX_CW)
	// Start continuous	wave transmission function expires after 65535 seconds
	lora_test_cw(lora_dev, config.frequency, config.tx_power,	65535);

	while(1) {
		LOG_INF("Continuous	Wave Tx	active.");
		k_sleep(K_MSEC(1000));
	}
#else
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
		int16_t rssi;
		int8_t snr;
		int len	= lora_recv(lora_dev, rxData,	MAX_RX_DATA_LEN, K_MSEC(5000),
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

