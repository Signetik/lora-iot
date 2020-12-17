/*
 * Copyright (c) 2016 Intel	Corporation
 *
 * SPDX-License-Identifier:	Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/lora.h>
#include <drivers/gpio.h>

#define	DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE,	okay),
		 "No default LoRa radio	specified in DT");
#define	DEFAULT_RADIO DT_LABEL(DEFAULT_RADIO_NODE)

#define	MAX_TX_DATA_LEN	10
#define	MAX_RX_DATA_LEN	255
//#define	TX_CW

#define	LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lora_send);

char txData[MAX_TX_DATA_LEN] = {'h', 'e', 'l', 'l',	'o', 'w', 'o', 'r',	'l', 'd'};
uint8_t	rxData[MAX_RX_DATA_LEN]	= {0};
const struct device	*lora_dev;
struct lora_modem_config config;

void main(void)
{
	int	ret, len;
	int16_t	rssi;
	int8_t snr;
	struct device *dev0;
	struct device *dev1;
	dev0 = device_get_binding("GPIO_0");
	dev1 = device_get_binding("GPIO_1");
	gpio_pin_configure(dev0, 9,	GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(dev1, 2,	GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(dev1, 4,	GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(dev1, 6,	GPIO_OUTPUT_ACTIVE);
	gpio_pin_set(dev0, 9,	0);
	gpio_pin_set(dev1, 2,	1);
	gpio_pin_set(dev1, 4,	0);
	gpio_pin_set(dev1, 6,	1);

	lora_dev = device_get_binding(DEFAULT_RADIO);
	if (!lora_dev) {
		LOG_ERR("%s	Device not found", DEFAULT_RADIO);
		return;
	}

	config.frequency = 915000000;
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
		ret	= lora_send(lora_dev, txData,	MAX_TX_DATA_LEN);
		if (ret	< 0) {
			LOG_ERR("LoRa send failed");
		}
		else {
			LOG_INF("Data sent!");
		}

		/* Block until data	arrives	or 5 seconds	passes */
		len	= lora_recv(lora_dev, rxData,	MAX_RX_DATA_LEN, K_MSEC(5000),
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
