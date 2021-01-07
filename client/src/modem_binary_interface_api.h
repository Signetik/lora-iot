/*============================================================================*
 *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                      SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#ifndef __MODEM_BINARY_INTERFACE_API_H
#define __MODEM_BINARY_INTERFACE_API_H

#include "modem_interface_api.h"

struct modem_info_s {
	uint8_t imei[32];
	uint32_t devid;
	uint8_t state;
};

enum state_s {
	ST_IDLE = 0,
	ST_LEN1,
	ST_LEN2,
	ST_PAYLOAD,
	ST_CHECK1,
	ST_CHECK2,
};

struct modem_binary_state_s {
    struct modem_state_s s;
	enum state_s state;
	uint8_t command[572];
	uint16_t checksum;
	uint16_t length;
	uint16_t offset;
	
	uint32_t tx_token, rx_token;
	struct modem_info_s info;
	uint32_t ip;
};

struct modem_api_func* modem_binary_api_get_func(void);

#endif /* __MODEM_BINARY_INTERFACE_API_H */
