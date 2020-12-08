/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier:	Apache-2.0
 */

#include <device.h>
#include <drivers/lora.h>
#include <errno.h>
#include <sys/util.h>
#include <zephyr.h>

#define	DEFAULT_RADIO DT_INST_0_SEMTECH_SX1262_LABEL
#define	MAX_TX_DATA_LEN	10
#define	MAX_RX_DATA_LEN	255
#define	TX_CW


#define	LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lora_send);

char txData[MAX_TX_DATA_LEN] = {'h', 'e', 'l', 'l',	'o', 'w', 'o', 'r',	'l', 'd'};
u8_t rxData[MAX_RX_DATA_LEN] = {0};
const struct device	*lora_dev;
struct lora_modem_config config;

/*!
 * Radio events	function pointer
 */
//static RadioEvents_t RadioEvents;

/*!
 * \brief Function executed	on Radio Tx	Timeout	event
 */
void OnRadioTxTimeout( void	)
{
	// Restarts	continuous wave	transmission when timeout expires after	65535 seconds
	lora_test_cw(lora_dev, config.frequency, config.tx_power,	65535);
}
 
void main(void)
{
	int	ret, len;
	s16_t rssi;
	s8_t snr;

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
	config.tx_power	= 4;
	config.tx =	true;

	ret	= lora_config(lora_dev,	&config);
	if (ret	< 0) {
		LOG_ERR("LoRa config failed");
		return;
	}
#if	defined(TX_CW)
	// Radio initialization
//	RadioEvents.TxTimeout =	OnRadioTxTimeout;
//	Radio.Init(	&RadioEvents );
 
	// Start continuous	wave transmission function expires after 65535 seconds
	lora_test_cw(lora_dev, config.frequency, config.tx_power,	65535);
	while(1) {
		LOG_INF("Continuous	Wave Tx	active.");
		k_sleep(K_MSEC(1000));
	}
#else
	while (1) {
		ret	= lora_send(lora_dev, data,	MAX_DATA_LEN);
		if (ret	< 0) {
			LOG_ERR("LoRa send failed");
		}
		else {
			LOG_INF("Data sent!");
		}

		/* Block until data	arrives	or 1 second	passes */
		len	= lora_recv(lora_dev, data,	MAX_DATA_LEN, K_MSEC(1000),
				&rssi, &snr);

		if (len	< 0) {
			LOG_ERR("LoRa receive failed");
		}

		if (len	> 0) {
			LOG_INF("Received data:	%s (RSSI:%ddBm,	SNR:%ddBm)",
			log_strdup(data), rssi,	snr);

		}
	}
#endif
}
