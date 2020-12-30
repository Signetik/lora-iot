/*============================================================================*
 *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                      SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#if 0
#include <zephyr.h>
#include <console/tty.h>
#include <logging/log.h>
#include <net/socket.h>
#include <fcntl.h>
#else
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#endif

#include "modem_binary_interface_api_shared.h"

static int modem_binary_check_data(uint16_t checksum);

void modem_binary_init(struct modem_state_s *s, uart_send_f uart_send, uart_recv_f uart_recv)
{
	struct modem_binary_state_s *state = (struct modem_binary_state_s*)s;
	
	state->tx_token = state->rx_token = 0;	
}

int modem_binary_process_byte(struct modem_state_s *s, char data)
{
	struct modem_binary_state_s *state = (struct modem_binary_state_s*)s;

	switch (state->state) {
		default:
		case ST_IDLE:
			if (data == MODEM_START_BYTE[0]) {
				state->state = ST_LEN1;
				memset(state->command, 0, sizeof(state->command));
			}
			break;
		case ST_LEN1:
			state->length = data;
			state->state = ST_LEN2;
			break;
		case ST_LEN2:
			state->length = state->length | ((uint16_t)data) << 8;
			state->offset = 0;
			state->state = ST_PAYLOAD;
			if (state->length > sizeof(state->command)) {
				state->state = ST_IDLE;
				modem_binary_send_error(state, 1);
			}
			break;
		case ST_PAYLOAD:
			// TODO: Handle max payload and abort character
			state->command[state->offset++] = data;
			if (state->offset == state->length)
				state->state = ST_CHECK1;
			break;
		case ST_CHECK1:
			state->checksum = data;
			state->state = ST_CHECK2;
			break;
		case ST_CHECK2:
			state->checksum = (state->checksum << 8) | data;
			state->state = ST_IDLE;
			if (modem_binary_check_data(state->checksum) != 0) {
				modem_binary_send_error(state, 2);
			}
			else {
				return modem_binary_parse(s, state->command, state->length);
			}
			break;
	}
	return 0;
}

static int modem_binary_check_data(uint16_t checksum)
{
	return 0;
}
