/*============================================================================*
 *		   Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved		  *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#include <zephyr.h>
#include <logging/log.h>
#include <net/socket.h>
#include <net/tls_credentials.h>
#include <fcntl.h>
#include <stdio.h>

#include "modem_ascii_interface_api.h"
#include "vars.h"
#include "sigconfig.h"

LOG_MODULE_REGISTER(modem_asc, LOG_LEVEL_DBG);

#define	MAX_CMD_LEN	16
#define	MAX_PARAMS 5

#define	START_BYTE_SENSOR '+'

static void	parse_command_sensor(struct	modem_ascii_state_s	*state);
static void	modem_ascii_init(struct	modem_state_s *s);
//static int modem_ascii_check_socket_receive(struct modem_state_s *s);
static int modem_ascii_process_byte(struct modem_state_s *s, char data);

static void	modem_ascii_send_error(struct modem_ascii_state_s *state, enum modem_result_code error);
static void	process_command(struct modem_ascii_state_s *state, struct command_s	*command);
static void	send_response_tty(struct modem_ascii_state_s *state, char *data);
//static uint32_t	server_resolve(char* address, enum net_ip_protocol proto);
//static bool	http_add_header(struct command_s *command, char** data,	size_t*	length,	char* method);
//static int http_add_message(struct command_s *command, char**	data,	size_t*	length,	bool opt_headers);

static void	ascii_handler_set(struct modem_ascii_state_s *state, struct	command_s *command);
static void	ascii_handler_get(struct modem_ascii_state_s *state, struct	command_s *command);
static void	ascii_handler_push(struct modem_ascii_state_s *state, struct command_s *command);
//static void	ascii_handler_udp_open(struct modem_ascii_state_s *state, struct command_s *command);
//static void	ascii_handler_udp_send(struct modem_ascii_state_s *state, struct command_s *command);
//static void	ascii_handler_udp_close(struct modem_ascii_state_s *state, struct command_s	*command);
//static void	ascii_handler_tcp_open(struct modem_ascii_state_s *state, struct command_s *command);
//static void	ascii_handler_tcp_send(struct modem_ascii_state_s *state, struct command_s *command);
//static void	ascii_handler_tcp_close(struct modem_ascii_state_s *state, struct command_s	*command);

//static void	ascii_handler_http_get(struct modem_ascii_state_s *state, struct command_s *command);
//static void	ascii_handler_http_put(struct modem_ascii_state_s *state, struct command_s *command);
//static void	ascii_handler_http_post(struct modem_ascii_state_s *state, struct command_s	*command);
//static void	ascii_handler_http_delete(struct modem_ascii_state_s *state, struct	command_s *command);
//static void	ascii_handler_http_patch(struct	modem_ascii_state_s	*state,	struct command_s *command);

static void	list_all_commands(struct modem_ascii_state_s *state);

static struct command_handler_s	handlers[] = {
	{"set",	ascii_handler_set},
	{"get",	ascii_handler_get},
	{"push", ascii_handler_push},
//	{"udp_open", ascii_handler_udp_open},
//	{"udp_send", ascii_handler_udp_send},
//	{"udp_close", ascii_handler_udp_close},
//	{"tcp_open", ascii_handler_tcp_open},
//	{"tcp_send", ascii_handler_tcp_send},
//	{"tcp_close", ascii_handler_tcp_close},
//	{"http_get", ascii_handler_http_get},
//	{"http_put", ascii_handler_http_put},
//	{"http_post", ascii_handler_http_post},
//	{"http_delete",	ascii_handler_http_delete},
//	{"http_patch", ascii_handler_http_patch},
	{NULL, NULL}
};

extern int64_t base_time;

#if	0
#define	MAX_RECORDS	24
struct custom_packet_storage_s
{
	uint32_t ts_s[MAX_RECORDS];
	uint32_t ts_ms[MAX_RECORDS];
	int32_t	values[MAX_RECORDS][10];
};

static struct custom_packet_storage_s custom_packet	= {	0 };

struct cell_packet_def_s sig_custom_packet_def =
{
	.packet_name	= "",
	.product_type	= "",
	.ts_s			= custom_packet.ts_s,
	.ts_ms			= custom_packet.ts_ms,
	.num_fields		= 0,
};

const cell_packet_def_t	*sig_default_packet_def	= &sig_custom_packet_def;
#endif

//+Custom,x_rms:3,y_rms:4,z_rms:7,temp_c:21,humidity:52

static struct modem_api_func modem_ascii_api_func =	{
	modem_ascii_init,
	NULL, //modem_ascii_check_socket_receive,
	modem_ascii_process_byte,
};

struct modem_api_func* modem_ascii_api_get_func(void) {
	return &modem_ascii_api_func;
};

static void	modem_ascii_init(struct	modem_state_s *s)
{
	//struct modem_ascii_state_s *state	= (struct modem_ascii_state_s*)s;
}

static int modem_ascii_process_byte(struct modem_state_s *s, char data)
{
	struct modem_ascii_state_s *state =	(struct	modem_ascii_state_s*)s;

	char echo_data[2] =	{ 0	};
	echo_data[0] = data;

	switch (state->state) {
		default:
		case ST_IDLE:
			if (var_echo)
				send_response_tty(state, echo_data);
			// Help	Menu
			if (data ==	'?') {
				list_all_commands(state);
			}
			if (data ==	START_BYTE_SENSOR) {
				state->length =	0;
				state->state = ST_PAYLOAD;
				memset(state->command, 0, sizeof(state->command));
				state->quoted =	false;
			}
			break;
		case ST_PAYLOAD:
			if (!state->quoted && (data	== '\r'	|| data	== '\n')) {
				if (var_echo)
					send_response_tty(state, "\r\n");
				parse_command_sensor(state);
				state->state = ST_IDLE;
			}
			else {
				// TODO: Add support for up	arrow (last	command)
				if (data ==	'"') {
					state->quoted =	!state->quoted;
				}
				// Backspace
				if (data ==	'\b') {
					if (state->length >	0) {
						state->command[--state->length]	= 0;
						if (var_echo) {
							send_response_tty(state, echo_data);
							send_response_tty(state, " ");
							send_response_tty(state, echo_data);
						}
					}
				}
				else {
					if (var_echo)
						send_response_tty(state, echo_data);
					if (state->length <	sizeof(state->command))	{
						state->command[state->length++]	= data;
					}
					else {
						modem_ascii_send_error(state, invalid_command);
						state->state = ST_IDLE;
					}
				}
			}
			break;
	}

	return 0;
}

static void	list_all_commands(struct modem_ascii_state_s *state)
{
	int	 num;
	char number[8];
	char command_str[MAX_KEY_LEN];

	send_response_tty(state, "\r\n\n");
	send_response_tty(state, "SigLRN Commands\r\n");
	send_response_tty(state, "----------------\r\n");

	while ((num	= list_next_command(command_str)))
	{
		// Align for 1 - 99	commands.
		snprintf(number, sizeof(number), "%2u) ", num);

		send_response_tty(state, number);
		send_response_tty(state, command_str);
		send_response_tty(state, "\r\n");
	}
}

static void	parse_command_sensor(struct	modem_ascii_state_s	*state)
{
	int	st = 0;
	int	index =	0;
	int	temp_pos;
	//static int value_index = 0;
	//static int record_number = 1;
	static struct command_s	command;
	int	pindex = 0;
	bool quoted	= false;

	for	(int p = 0 ; p <= VAR_MAX_FIELDS ; p++)	{
		command.parameters[p].value	= NULL;
	}

	command.param_count	= 0;
	//memset(sig_custom_packet_def.packet_name,	0, sizeof(sig_custom_packet_def.packet_name));
	//memset(sig_custom_packet_def.field, 0, sizeof(sig_custom_packet_def.field));

	state->command[state->length] =	0;
	state->length++;

	for	(int pos = 0 ; pos < state->length ; pos++)	{
		// TODO: Add check for param_count when	it exceeds the number of fields
		switch (st)	{
			case 0:
				if (state->command[pos]	== ',')	{
					if (index >	0) {
						command.command[index] = 0;
						st = 1;
						index =	0;
						temp_pos =	pos	+ 1;
					}
					else {
						modem_ascii_send_error(state, invalid_command);
						return;
					}
				}
				else if	(index < sizeof(command.command)) {
					command.command[index++] = state->command[pos];
				}
				break;
			case 1:
				if (state->command[pos]	== ':')	{
					if (index >	0) {
						command.parameters[command.param_count].key[index] = 0;
						st = 2;
						index =	0;
						temp_pos =	pos	+ 1;
					}
					else {
						modem_ascii_send_error(state, invalid_key);
						return;
					}
				}
				else if	(state->command[pos] ==	','	|| state->command[pos] == 0) {
					if (index >	0) {
						command.parameters[command.param_count].key[index] = 0;
						command.parameters[command.param_count].value =	NULL;
						command.parameters[command.param_count].vlen = 0;
						pindex++;
						command.param_count++;
						st = 1;
						index =	0;
					}
					else {
						modem_ascii_send_error(state, invalid_key);
						return;
					}
				}
				else if	(index < sizeof(command.parameters[command.param_count].key)) {
					command.parameters[command.param_count].key[index++] = state->command[pos];
				}
				break;
			case 2:
				if (state->command[pos]	== '"')	{
					quoted = !quoted;
				}
				if ((!quoted &&	state->command[pos]	== ',')	|| state->command[pos] == 0) {
					command.parameters[command.param_count].value =	&state->command[temp_pos];
					command.parameters[command.param_count].value[pos-temp_pos]	= 0;
					command.parameters[command.param_count].vlen = pos-temp_pos;
					command.param_count++;
					st = 1;
					index =	0;
				}
				else {
					index++;
				}
				break;
		}
	}

	process_command(state, &command);
#if	0
	if (field >	0) {
		if (base_time != 0)	{
			s64_t now =	k_uptime_get() + base_time;
			custom_packet.ts_s[value_index]	= (u32_t)(now /	1000);
			custom_packet.ts_ms[value_index] = (u32_t)(now % 1000);
		}
		else {
			custom_packet.ts_s[value_index]	= 0;
			custom_packet.ts_ms[value_index] = 0;
		}

		sig_custom_packet_def.num_fields = field;
		strcpy(sig_custom_packet_def.product_type, "SIGTHA");

		LTE_send();

		// wait	and	if success
		int	result = LTE_wait_complete(K_FOREVER);

		if (result == 0) {
			LOG_INF("Data sent to server. Updating stored data.");
			record_number += 1;
			state->s.uart_send("OK\r\n", 4);
		}
		else {
			LOG_ERR("Failed	to send	data. Dropping data.");
			// TODO:
			record_number += 1;
			modem_ascii_send_error(state, 200);
		}
	}
	else {
		modem_ascii_send_error(state, 110);
	}
#endif
}

static void	process_command(struct modem_ascii_state_s *state, struct command_s	*command)
{
	int	hindex = 0;

	while (handlers[hindex].command) {
		if (strcmp(handlers[hindex].command, command->command) == 0) {
			handlers[hindex].handler(state,	command);
			break;
		}
		hindex++;
	}
	if (!handlers[hindex].command) {
		modem_ascii_send_error(state, invalid_command);
	}
}

static void	ascii_handler_set(struct modem_ascii_state_s *state, struct	command_s *command)
{
	int	result;
	char *str_value;

	result = vars_set(command->parameters[0].key, command->parameters[0].value,	command->parameters[0].vlen, &str_value);

	if (result == verr_inv_value) {
		modem_ascii_send_error(state, invalid_value);
		return;
	}

	if (result == verr_inv_access) {
		modem_ascii_send_error(state, invalid_access);
		return;
	}

	if (result < 0)	{
		modem_ascii_send_error(state, invalid_key);
		return;
	}

	send_response_tty(state, "+rsp,");
	send_response_tty(state, command->parameters[0].key);
	send_response_tty(state, ":");
	send_response_tty(state, str_value ? str_value : "NULL");
	send_response_tty(state, "\r\n");
}

static void	ascii_handler_get(struct modem_ascii_state_s *state, struct	command_s *command)
{
	int	result;
	char *str_value;

	result = vars_get(command->parameters[0].key, command->parameters[0].value,	command->parameters[0].vlen, &str_value);

	if (result == verr_inv_value) {
		modem_ascii_send_error(state, invalid_value);
		return;
	}

	if (result == verr_inv_access) {
		modem_ascii_send_error(state, invalid_access);
		return;
	}

	if (result < 0)	{
		modem_ascii_send_error(state, invalid_key);
		return;
	}

	send_response_tty(state, "+rsp,");
	send_response_tty(state, command->parameters[0].key);
	send_response_tty(state, ":");
	send_response_tty(state, str_value ? str_value : "");
	send_response_tty(state, "\r\n");
}

static void	ascii_handler_push(struct modem_ascii_state_s *state, struct command_s *command)
{
	uint32_t sequence =	sigconfig_push(command->parameters[0].key, &command->parameters[1],	command->param_count-1,	k_uptime_get()/* + base_time*/ );
	char result[16];

	if (sequence > 0) {
		snprintf(result, sizeof(result), "%u", sequence);
		send_response_tty(state, "+rsp,result:0,mark:");
		send_response_tty(state, result);
		send_response_tty(state, "\r\n");
	}
	else {
		modem_ascii_send_error(state, invalid_report);
	}
}
#if	0
static void	ascii_handler_udp_open(struct modem_ascii_state_s *state, struct command_s *command)
{
	int	sock = 0;
	char socket[8];

	if (strcmp(command->parameters[0].key, "host") !=0 || strcmp(command->parameters[1].key, "port") !=0) {
		modem_ascii_send_error(state, invalid_key);
		return;
	}

	// Resolve server address
	uint32_t server_address	= server_resolve(command->parameters[0].value, IPPROTO_UDP);
	if (server_address == 0) {
		modem_ascii_send_error(state, invalid_value);
		LOG_ERR("Address could not be resolved.");
		return;
	}
	uint16_t server_port = atoi(command->parameters[1].value);

	// Connect to server
	sock = client_connect(&(state->s), server_address, server_port,	IPPROTO_UDP, 0);
	if (sock < 0) {
		modem_ascii_send_error(state, invalid_access);
		LOG_ERR("udp socket	failed to open.");
		return;
	}

	snprintf(socket, sizeof(socket), "%d", sock);
	send_response_tty(state, "+rsp,result:0,socket:");
	send_response_tty(state, socket);
	send_response_tty(state, "\r\n");
	LOG_INF("udp socket	open, socket id: %d", sock);
}
#endif
#if	0
static void	ascii_handler_udp_send(struct modem_ascii_state_s *state, struct command_s *command)
{
	int	sock = 0;
	uint16_t socki = 0;
	uint8_t	*data =	NULL;
	size_t length =	0;
	
	// Set socket value	if input is	formatted correctly.
	if (strcmp(command->parameters[0].key, "socket") ==	0) {
		sock = atoi(command->parameters[0].value);
		if (sock > 0) {
			socki =	(uint16_t)sock;
			if (state->s.sock_state[socki].ip_proto	!= IPPROTO_UDP)	 {
				modem_ascii_send_error(state, invalid_access);
				LOG_ERR("udp send failed, not a	udp	socket.");
				return;
			}
		}
		else {
			modem_ascii_send_error(state, invalid_value);
			LOG_ERR("udp send failed, invalid socket id.");
			return;
		}
	}
	else {
		modem_ascii_send_error(state, invalid_key);
		return;
	}

	// Set data	value and length if	input is formatted correctly.
	if (strcmp(command->parameters[1].key, "data") == 0) {
		data = command->parameters[1].value;
		length = command->parameters[1].vlen;
	}
	else {
		modem_ascii_send_error(state, invalid_key);
		return;
	}
	
	// Remove quotes from buffer, if quoted
	if (data[0]	== '"')	{
		data++;				//Skip the first "
		length = length-2;		//Compsensate for lost chars
		data[length] = '\0';		//Add NULL to end
	}

	// Send	data
	int	result = client_send(&(state->s), socki, data, length);
	if (result < 0)	{
		modem_ascii_send_error(state, invalid_access);
		LOG_ERR("udp send failed: %d", result);
		return;
	}

	send_response_tty(state, "+rsp,result:0\r\n");
	LOG_INF("udp send successful.");
}
#endif
#if	0
static void	ascii_handler_udp_close(struct modem_ascii_state_s *state, struct command_s	*command)
{
	int	sock = 0;
	if (strcmp(command->parameters[0].key, "socket") ==	0) {
		sock = atoi(command->parameters[0].value);
		
		// Verify protocol
		if (state->s.sock_state[sock].ip_proto != IPPROTO_UDP){
			modem_ascii_send_error(state, invalid_access);
			LOG_ERR("Not a udp socket.");
			return;
		}
		
		// Disconnect socket
		int	res	= client_disconnect(&(state->s), sock);
		if (res	< 0) {
			modem_ascii_send_error(state, -res);
			LOG_ERR("udp socket	failed to close.");
			return;
		}
	}
	else {
		modem_ascii_send_error(state, invalid_key);
		return;
	}

	send_response_tty(state, "+rsp,result:0\r\n");
	LOG_INF("udp socket	%d closed.", sock);
}
#endif
#if	0
static void	ascii_handler_tcp_open(struct modem_ascii_state_s *state, struct command_s *command)
{
	int	sock = 0;
	char socket[8];

	if (strcmp(command->parameters[0].key, "host") !=0 || strcmp(command->parameters[1].key, "port") !=0) {
		modem_ascii_send_error(state, invalid_key);
		return;
	}
	
	// Resolve server address
	uint32_t server_address	= server_resolve(command->parameters[0].value, IPPROTO_TCP);
	if (server_address == 0) {
		modem_ascii_send_error(state, invalid_value);
		LOG_ERR("Address could not be resolved.");
		return;
	}
	uint16_t server_port = atoi(command->parameters[1].value);

	// Connect to server
	sock = client_connect(&(state->s), server_address, server_port,	IPPROTO_TCP, var_sectag);
	if (sock < 0) {
		modem_ascii_send_error(state, invalid_access);
		LOG_ERR("tcp socket	failed to open.");
		return;
	}

	snprintf(socket, sizeof(socket), "%d", sock);
	send_response_tty(state, "+rsp,result:0,socket:");
	send_response_tty(state, socket);
	send_response_tty(state, "\r\n");
	LOG_INF("tcp socket	open, socket id: %d", sock);
}
#endif
#if	0
static void	ascii_handler_tcp_send(struct modem_ascii_state_s *state, struct command_s *command)
{
	int	sock = 0;
	uint16_t socki = 0;
	uint8_t	*data =	NULL;
	size_t length =	0;
	
	// Set socket value	if input is	formatted correctly.
	if (strcmp(command->parameters[0].key, "socket") ==	0) {
		sock = atoi(command->parameters[0].value);
		if (sock > 0) {
			socki =	(uint16_t)sock;
			if (state->s.sock_state[socki].ip_proto	!= IPPROTO_TCP)	 {
				modem_ascii_send_error(state, invalid_access);
				LOG_ERR("tcp send failed, not a	tcp	socket.");
				return;
			}
		}
		else {
			modem_ascii_send_error(state, invalid_value);
			LOG_ERR("tcp send failed, invalid socket id.");
			return;
		}
	}
	else {
		modem_ascii_send_error(state, invalid_key);
		return;
	}

	// Set data	value and length if	input is formatted correctly.
	if (strcmp(command->parameters[1].key, "data") == 0) {
		data = command->parameters[1].value;
		length = command->parameters[1].vlen;
	}
	else {
		modem_ascii_send_error(state, invalid_key);
		return;
	}
	
	// Remove quotes from buffer, if quoted
	if (data[0]	== '"')	{
		data++;				//Skip the first "
		length = length-2;		//Compsensate for lost chars
		data[length] = '\0';		//Add NULL to end
	}

	// Send	data
	int	result = client_send(&(state->s), socki, data, length);
	if (result < 0)	{
		modem_ascii_send_error(state, invalid_access);
		LOG_ERR("tcp send failed: %d", result);
		return;
	}

	send_response_tty(state, "+rsp,result:0\r\n");
	LOG_INF("tcp send successful.");
}
#endif
#if	0
static void	ascii_handler_tcp_close(struct modem_ascii_state_s *state, struct command_s	*command)
{
	int	sock = 0;
	if (strcmp(command->parameters[0].key, "socket") ==	0) {
		sock = atoi(command->parameters[0].value);
		
		// Verify protocol
		if (state->s.sock_state[sock].ip_proto != IPPROTO_TCP){
			modem_ascii_send_error(state, invalid_access);
			LOG_ERR("Not a tcp socket.");
			return;
		}
		
		// Disconnect socket
		int	res	= client_disconnect(&(state->s), sock);
		if (res	< 0) {
			modem_ascii_send_error(state, -res);
			LOG_ERR("tcp socket	failed to close.");
			return;
		}
	}
	else {
		modem_ascii_send_error(state, invalid_key);
		return;
	}

	send_response_tty(state, "+rsp,result:0\r\n");
	LOG_INF("tcp socket	%d closed.", sock);
}
#endif
#if	0
static void	ascii_handler_http_get(struct modem_ascii_state_s *state, struct command_s *command)
{
	int	sock = 0;
	
	// Set socket value	if input is	formatted correctly.
	if (strcmp(command->parameters[0].key, "socket") ==	0) {
		sock = atoi(command->parameters[0].value);
		if (sock <=	0) {
			modem_ascii_send_error(state, invalid_value);
			return;
		}
	}
	else {
		modem_ascii_send_error(state, invalid_key);
		return;
	}
	
	// Fill	in header data.
	size_t length =	0;
	char* data = malloc(sizeof(struct http_request_header) + MAX_MESSAGE_LEN);
	bool opt_headers = http_add_header(command,	&data, &length,	"GET");

	// Send	data.
	int	result = client_send(&(state->s), sock,	data, length);
	if (result < 0)	{
		modem_ascii_send_error(state, invalid_access);
		LOG_ERR("HTTP GET send failed: %d",	result);
		free(data);
		return;
	}

	send_response_tty(state, "+rsp,result:0\r\n");
	LOG_INF("HTTP GET send successful.");
	free(data);
}
#endif
#if	0
static void	ascii_handler_http_put(struct modem_ascii_state_s *state, struct command_s *command)
{
	int	sock = 0;
	
	// Set socket value	if input is	formatted correctly.
	if (strcmp(command->parameters[0].key, "socket") ==	0) {
		sock = atoi(command->parameters[0].value);
		if (sock <=	0) {
			modem_ascii_send_error(state, invalid_value);
			return;
		}
	}
	else {
		modem_ascii_send_error(state, invalid_key);
		return;
	}
	
	// Fill	in header data.
	size_t length =	0;
	char* data = malloc(sizeof(struct http_request_header) + MAX_MESSAGE_LEN);
	bool opt_headers = http_add_header(command,	&data, &length,	"PUT");
	
	// Append message to header	data.
	int	result = http_add_message(command, &data, &length, opt_headers);
	if (result != 0) {
		modem_ascii_send_error(state, invalid_key);
		LOG_ERR("HTTP PUT append message failed: %d", result);
		free(data);
		return;
	}
	
	// Send	data.
	result = client_send(&(state->s), sock,	data, length);
	if (result < 0)	{
		modem_ascii_send_error(state, invalid_access);
		LOG_ERR("HTTP PUT send failed: %d",	result);
		free(data);
		return;
	}

	send_response_tty(state, "+rsp,result:0\r\n");
	LOG_INF("HTTP PUT send successful.");
	free(data);
}
#endif
#if	0
static void	ascii_handler_http_post(struct modem_ascii_state_s *state, struct command_s	*command)
{
	int	sock = 0;
	
	// Set socket value	if input is	formatted correctly.
	if (strcmp(command->parameters[0].key, "socket") ==	0) {
		sock = atoi(command->parameters[0].value);
		if (sock <=	0) {
			modem_ascii_send_error(state, invalid_value);
			return;
		}
	}
	else {
		modem_ascii_send_error(state, invalid_key);
		return;
	}
	
	// Fill	in header data.
	size_t length =	0;
	char* data = malloc(sizeof(struct http_request_header) + MAX_MESSAGE_LEN);
	bool opt_headers = http_add_header(command,	&data, &length,	"POST");
	
	// Append message to header	data.
	int	result = http_add_message(command, &data, &length, opt_headers);
	if (result != 0) {
		modem_ascii_send_error(state, invalid_key);
		LOG_ERR("HTTP POST append message failed: %d", result);
		free(data);
		return;
	}
	
	// Send	data.
	result = client_send(&(state->s), sock,	data, length);
	if (result < 0)	{
		modem_ascii_send_error(state, invalid_access);
		LOG_ERR("HTTP POST send	failed:	%d", result);
		free(data);
		return;
	}

	send_response_tty(state, "+rsp,result:0\r\n");
	LOG_INF("HTTP POST send	successful.");
	free(data);
}
#endif
#if	0
static void	ascii_handler_http_delete(struct modem_ascii_state_s *state, struct	command_s *command)
{
	int	sock = 0;
	
	// Set socket value	if input is	formatted correctly.
	if (strcmp(command->parameters[0].key, "socket") ==	0) {
		sock = atoi(command->parameters[0].value);
		if (sock <=	0) {
			modem_ascii_send_error(state, invalid_value);
			return;
		}
	}
	else {
		modem_ascii_send_error(state, invalid_key);
		return;
	}
	
	// Fill	in header data.
	size_t length =	0;
	char* data = malloc(sizeof(struct http_request_header) + MAX_MESSAGE_LEN);
	bool opt_headers = http_add_header(command,	&data, &length,	"DELETE");

	// Send	data.
	int	result = client_send(&(state->s), sock,	data, length);
	if (result < 0)	{
		modem_ascii_send_error(state, invalid_access);
		LOG_ERR("HTTP DELETE send failed: %d", result);
		free(data);
		return;
	}

	send_response_tty(state, "+rsp,result:0\r\n");
	LOG_INF("HTTP DELETE send successful.");
	free(data);
}
#endif
#if	0
static void	ascii_handler_http_patch(struct	modem_ascii_state_s	*state,	struct command_s *command)
{
	int	sock = 0;
	
	// Set socket value	if input is	formatted correctly.
	if (strcmp(command->parameters[0].key, "socket") ==	0) {
		sock = atoi(command->parameters[0].value);
		if (sock <=	0) {
			modem_ascii_send_error(state, invalid_value);
			return;
		}
	}
	else {
		modem_ascii_send_error(state, invalid_key);
		return;
	}
	
	// Fill	in header data.
	size_t length =	0;
	char* data = malloc(sizeof(struct http_request_header) + MAX_MESSAGE_LEN);
	bool opt_headers = http_add_header(command,	&data, &length,	"PATCH");
	
	// Append message to header	data.
	int	result = http_add_message(command, &data, &length, opt_headers);
	if (result != 0) {
		modem_ascii_send_error(state, invalid_key);
		LOG_ERR("HTTP PATCH	append message failed: %d",	result);
		free(data);
		return;
	}

	// Send	data.
	result = client_send(&(state->s), sock,	data, length);
	if (result < 0)	{
		modem_ascii_send_error(state, invalid_access);
		LOG_ERR("HTTP PATCH	send failed: %d", result);
		free(data);
		return;
	}

	send_response_tty(state, "+rsp,result:0\r\n");
	LOG_INF("HTTP PATCH	send successful.");
	free(data);
}
#endif
#if	0
static bool	http_add_header(struct command_s *command, char** data,	size_t*	length,	char* method)
{
	bool opt_headers;
	
	// Set optional	headers	values if input	is formatted correctly.
	if (strcmp(command->parameters[1].key, "opt_headers") == 0)	{
		
		// Remove quotes from optional headers,	if quoted.
		if (command->parameters[1].value[0]	== '"')	{
			command->parameters[1].value++;
			command->parameters[1].value[strlen(command->parameters[1].value) -	1] = '\0';
		}
		
		// Fill	in header WITH optional	header values.
		*length	= snprintf(*data, sizeof(struct	http_request_header), HTTP_HEADER_TEMPLATE_OPTIONS,
				method,	var_uri.data,
				var_host.data,
				var_keepalive ?	"keep-alive" : "close",
				command->parameters[1].value);

		opt_headers	= true;
	}
	else {
		// Fill	in header WITHOUT optional header values.
		*length	= snprintf(*data, sizeof(struct	http_request_header) - MAX_OPTIONAL_HEADER_LEN,	HTTP_HEADER_TEMPLATE,
				method,	var_uri.data,
				var_host.data,
				var_keepalive ?	"keep-alive" : "close");

		opt_headers	= false;
	}
	
	LOG_INF("HTTP header length: %d", *length);
	return opt_headers;
}
#endif
#if	0
static int http_add_message(struct command_s *command, char** data,	size_t*	length,	bool opt_headers)
{
	int	idx;
	
	// Determine which parameter value to use based	on if optional headers were	given.
	if (opt_headers	== false) {
		idx	= 1;
	}
	else {
		idx	= 2;
	}
	
	// Fill	in message if input	is formatted correctly.
	if (strcmp(command->parameters[idx].key, "data") ==	0) {
			
		// Remove quotes from message, if quoted.
		if (command->parameters[idx].value[0] == '"') {
			command->parameters[idx].value++;
			command->parameters[idx].value[strlen(command->parameters[idx].value) -	1] = '\0';
		}
			
		// Append message to header	data.
		strcat(*data, command->parameters[idx].value);
		*length	= *length +	strlen(command->parameters[idx].value);
	}
	else {
		return -1;
	}
	
	LOG_INF("HTTP total	length:	%d", *length);
	return 0;
}
#endif
static void	modem_ascii_send_error(struct modem_ascii_state_s *state, enum modem_result_code error)
{
	char result[4];
	snprintf(result, sizeof(result), "%u", error);

	send_response_tty(state, "+rsp,result:");
	send_response_tty(state, result);
	send_response_tty(state, "\r\n");
}

static void	send_response_tty(struct modem_ascii_state_s *state, char *data)
{
	state->s.uart_send(data, strlen(data));
}
#if	0
static int modem_ascii_check_socket_receive(struct modem_state_s *s)
{

	struct modem_ascii_state_s *state =	(struct	modem_ascii_state_s*)s;
	int	off;
	int	bytes =	0;
	char received[MAX_REC_LEN];

	for	(int sindex	= 0	; sindex < MAX_MODEM_SOCKET	; sindex++)	{
		int	sock = state->s.sock_state[sindex].sock;
		if (sock > 0) {

			// Clear receive buffer	and	offset
			memset(received, 0,	sizeof(received));
			off	= 0;

			// Read	data
			do {
				bytes =	recv(sock, &received[off], sizeof(received)-off, 0);
				if (bytes >	0) {
					off	+= bytes;
				}
			} while	(bytes > 0);

			// Respond received	data, if any.
			if (off	> 0) {
//				k_sem_take(&sem_uart, K_FOREVER);
//				state->s.sock_state[sindex].ts = k_uptime_get();
//				k_sem_give(&sem_uart);

				switch (state->s.sock_state[sindex].ip_proto) {
					case IPPROTO_UDP:
						send_response_tty(state, "+notify,event:udp_receive,data:\n");
						break;
					case IPPROTO_TCP:
						send_response_tty(state, "+notify,event:tcp_receive,data:\n");
						break;
					default:
						LOG_ERR("Protocol not supported.");
						return -ENOTSUP;
						break;
				}

				send_response_tty(state, &received);
				send_response_tty(state, "\r\n");
			}
		}
	}

	return 0;

}
#endif
#if	0
static uint32_t	server_resolve(char* address, enum net_ip_protocol proto)
{
	struct addrinfo	*res;
	struct addrinfo	hints =	{
		.ai_family = AF_INET,
	};
	switch (proto) {
		case IPPROTO_UDP:
			hints.ai_socktype =	SOCK_DGRAM;	break;
		case IPPROTO_TCP:
			hints.ai_socktype =	SOCK_STREAM; break;
		default:
			return 0; break;
	}

	// Resolve server name or address string.
	if (getaddrinfo(address, NULL, &hints, &res) !=	0) {
		LOG_ERR("getaddrinfo() failed, err %d\n", errno);
		return 0;
	}
	
	// Convert to host byte	order to avoid conflict	with client_connect() later.
	return ntohl(((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr);
}
#endif