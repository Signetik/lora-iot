/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#ifndef	__MODEM_ASCII_INTERFACE_API_H
#define	__MODEM_ASCII_INTERFACE_API_H

#include "modem_interface_api.h"
#include "sigconfig.h"

enum state_s {
	ST_IDLE	= 0,
	ST_PAYLOAD,
};

struct modem_ascii_state_s {
	struct modem_state_s s;
	enum state_s state;
	uint8_t	command[4096];
	uint16_t checksum;
	uint16_t length;
	uint16_t offset;
	bool quoted;
};

struct command_s {
	uint8_t	command[16];
	int	param_count;
	struct var_param_s parameters[VAR_MAX_FIELDS+1];
};

struct command_handler_s {
	const uint8_t *command;
	void (*handler)(struct modem_ascii_state_s *state, struct command_s	*command);
};

struct modem_api_func* modem_ascii_api_get_func(void);

#endif /* __MODEM_ASCII_INTERFACE_API_H	*/
