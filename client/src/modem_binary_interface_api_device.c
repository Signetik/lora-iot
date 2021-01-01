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
#include <fcntl.h>

#include "modem_binary_interface_api_shared.h"
#include "vars.h"
#include "sigconfig.h"
#include "lora_task.h"

LOG_MODULE_REGISTER(modem_bin, LOG_LEVEL_DBG);

void modem_binary_init(struct modem_state_s	*state);
//int modem_binary_check_socket_receive(struct modem_state_s *state);
//int modem_binary_check_socket_send(struct	modem_state_s *state);
int	modem_binary_process_byte(struct modem_state_s *state, char	data);
int	modem_binary_parse(struct modem_state_s	*state,	uint8_t	*data, int data_length);

void modem_binary_send_error(struct	modem_binary_state_s *state, char error);

static struct modem_api_func modem_binary_api_func = {
	modem_binary_init,
	NULL, //modem_binary_check_socket_receive,
	modem_binary_process_byte,
//	modem_binary_parse,
};

struct modem_api_func* modem_binary_api_get_func(void) {
	return &modem_binary_api_func;
};

static void	send_response_tty(struct modem_binary_state_s *state, struct modem_response_s *response, uint16_t length);
static int check_command_size(int buffer_length, int payload_length);

struct command_handler_s {
	const uint8_t command;
	void (*handler)(struct modem_binary_state_s	*state,	struct modem_command_s *cmd, struct	modem_response_s *resp);
	int	size;
};

//static void	binary_handler_info(struct modem_binary_state_s	*state,	struct modem_command_s *cmd, struct	modem_response_s *resp);
static void	binary_handler_set(struct modem_binary_state_s *state, struct modem_command_s *cmd,	struct modem_response_s	*resp);
static void	binary_handler_get(struct modem_binary_state_s *state, struct modem_command_s *cmd,	struct modem_response_s	*resp);
static void	binary_handler_push(struct modem_binary_state_s	*state,	struct modem_command_s *cmd, struct	modem_response_s *resp);

//static void binary_handler_udp_open(struct modem_binary_state_s *state, struct modem_command_s *cmd, struct modem_response_s *resp);
//static void binary_handler_udp_send(struct modem_binary_state_s *state, struct modem_command_s *cmd, struct modem_response_s *resp);
//static void binary_handler_udp_close(struct modem_binary_state_s *state, struct modem_command_s *cmd,	struct modem_response_s	*resp);
//static void binary_handler_tcp_open(struct modem_binary_state_s *state, struct modem_command_s *cmd, struct modem_response_s *resp);
//static void binary_handler_tcp_send(struct modem_binary_state_s *state, struct modem_command_s *cmd, struct modem_response_s *resp);
//static void binary_handler_tcp_close(struct modem_binary_state_s *state, struct modem_command_s *cmd,	struct modem_response_s	*resp);
static	void binary_handler_recv(struct	modem_binary_state_s *state, struct	modem_command_s	*cmd, struct modem_response_s *resp);
//static void binary_handler_udp_resolve(struct	modem_binary_state_s *state, struct	modem_command_s	*cmd, struct modem_response_s *resp);
//static void binary_handler_tcp_resolve(struct	modem_binary_state_s *state, struct	modem_command_s	*cmd, struct modem_response_s *resp);

//static void binary_handler_http_get(struct modem_binary_state_s *state, struct modem_command_s *cmd, struct modem_response_s *resp);
//static void binary_handler_http_put(struct modem_binary_state_s *state, struct modem_command_s *cmd, struct modem_response_s *resp);
//static void binary_handler_http_post(struct modem_binary_state_s *state, struct modem_command_s *cmd,	struct modem_response_s	*resp);
//static void binary_handler_http_delete(struct	modem_binary_state_s *state, struct	modem_command_s	*cmd, struct modem_response_s *resp);
//static void binary_handler_http_patch(struct modem_binary_state_s	*state,	struct modem_command_s *cmd, struct	modem_response_s *resp);

//static void binary_handler_socket_status(struct modem_binary_state_s *state, struct modem_command_s *cmd,	struct modem_response_s	*resp);
//static void binary_handler_close_all_sockets(struct modem_binary_state_s *state, struct modem_response_s *resp);

static struct command_handler_s	handlers[] = {
//	{MODEM_CMD_INFO, binary_handler_info, sizeof(struct	info_s)},
	{MODEM_CMD_SET,	binary_handler_set,	sizeof(struct set_s)},
	{MODEM_CMD_GET,	binary_handler_get,	sizeof(struct get_s)},
	{MODEM_CMD_PUSH, binary_handler_push, sizeof(struct	push_s)},
//	{MODEM_CMD_UDP_OPEN, binary_handler_udp_open, sizeof(struct	udp_open_s)},
//	{MODEM_CMD_UDP_SEND, binary_handler_udp_send, sizeof(struct	send_s)},
//	{MODEM_CMD_UDP_CLOSE, binary_handler_udp_close,	sizeof(struct close_s)},
//	{MODEM_CMD_TCP_OPEN, binary_handler_tcp_open, sizeof(struct	tcp_open_s)},
//	{MODEM_CMD_TCP_SEND, binary_handler_tcp_send, sizeof(struct	send_s)},
//	{MODEM_CMD_TCP_CLOSE, binary_handler_tcp_close,	sizeof(struct close_s)},
	{MODEM_CMD_RECV, binary_handler_recv, sizeof(struct	recv_s)},
//	{MODEM_CMD_UDP_RESOLVE,	binary_handler_udp_resolve,	sizeof(struct resolve_s)},
//	{MODEM_CMD_TCP_RESOLVE,	binary_handler_tcp_resolve,	sizeof(struct resolve_s)},
//	{MODEM_CMD_HTTP_GET, binary_handler_http_get, sizeof(struct	http_get_s)},
//	{MODEM_CMD_HTTP_PUT, binary_handler_http_put, sizeof(struct	http_put_s)},
//	{MODEM_CMD_HTTP_POST, binary_handler_http_post,	sizeof(struct http_post_s)},
//	{MODEM_CMD_HTTP_DELETE,	binary_handler_http_delete,	sizeof(struct http_delete_s)},
//	{MODEM_CMD_HTTP_PATCH, binary_handler_http_patch, sizeof(struct	http_patch_s)},
//	{MODEM_CMD_SOCKET_STATUS, binary_handler_socket_status,	sizeof(struct sock_stat_s)},
//	{MODEM_CMD_CLOSE_ALL_SOCKETS, binary_handler_close_all_sockets,	sizeof(struct info_s)},
	{0,	NULL}
};

//extern u8_t imei[32];
//extern u32_t device_id;

#if	defined(FIFO)
struct send_item_s {
	void *fifo_reserved;   /* 1st word reserved	for	use	by fifo	*/
	uint8_t	in_use;
	uint8_t	length;
	uint8_t	sock;
	uint8_t	data[128];
};

struct send_item_s send_items[16];

K_FIFO_DEFINE(send_fifo);
#endif

int	modem_binary_parse(struct modem_state_s	*s,	uint8_t	*data, int data_length)
{
	struct modem_command_s *cmd	= (struct modem_command_s*)data;
	struct modem_response_s	*resp =	calloc(1, sizeof(struct	modem_response_s));
	struct modem_binary_state_s	*state = (struct modem_binary_state_s*)s;
	int	hindex = 0;

	memcpy(&resp->header.token,	&cmd->header.token,	sizeof(cmd->header.token));

	while (handlers[hindex].command) {
		if (handlers[hindex].command ==	cmd->header.command) {
			if (!check_command_size(data_length, handlers[hindex].size)) {
				modem_binary_send_error(state, 102);
				free(resp);
				return 102;
			}
			handlers[hindex].handler(state,	cmd, resp);
			break;
		}
		hindex++;
	}
	if (!handlers[hindex].command) {
		modem_binary_send_error(state, invalid_command);
	}
	
	free(resp);
	return 0;
}
#if	0
static void	binary_handler_info(struct modem_binary_state_s	*state,	struct modem_command_s *command, struct	modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;

	LOG_INF("MODEM_CMD_INFO");
	response.header.response = MODEM_RSP_INFO;
	response.header.result = 0;

	memcpy(response.payload.info.imei, imei, sizeof(response.payload.info.imei));
	response.payload.info.devid	= var_device_id;
	response.payload.info.state	= LTE_is_connected() ? 0x01	: 0x00;
	length = sizeof(struct header_response_s) +	sizeof(response.payload.info);
	send_response_tty(state, &response,	length);
}
#endif
static void	binary_handler_set(struct modem_binary_state_s *state, struct modem_command_s *cmd,	struct modem_response_s	*resp)
{
	int	result;
	uint16_t length;
	struct modem_response_s	response = *resp;
	
	// Get command data.
	char* var_name = (char*)cmd->payload.set.var_name;
	char* value	= (char*)cmd->payload.set.value;
	int	vlen = strlen(value)+1;
	char* str_value;
	
	LOG_INF("MODEM_CMD_SET:	var_name: %s, value: %s", log_strdup(var_name),	log_strdup(value));
	
	result = vars_set(var_name,	value, vlen, &str_value);
	if (result == verr_inv_value) {
		modem_binary_send_error(state, invalid_value);
		return;
	}
	if (result == verr_inv_access) {
		modem_binary_send_error(state, invalid_access);
		return;
	}
	if (result < 0)	{
		modem_binary_send_error(state, invalid_key);
		return;
	}

	// Fill	in response	data.
	response.header.response = MODEM_RSP_SET;
	response.header.result = result;

	length = sizeof(struct header_response_s);
	LOG_INF("MODEM_RSP_SET:	result:	%d", response.header.result);
	send_response_tty(state, &response,	length);
}

static void	binary_handler_get(struct modem_binary_state_s *state, struct modem_command_s *cmd,	struct modem_response_s	*resp)
{
	int	result;
	uint16_t length;
	struct modem_response_s	response = *resp;

	// Get command data.
	char* var_name = (char*)cmd->payload.get.var_name;
	char value[] = {0};
	char* str_value;
	
	LOG_INF("MODEM_CMD_GET:	var_name: %s", log_strdup(var_name));
	
	result = vars_get(var_name,	value, 0, &str_value);
	if (result == verr_inv_value) {
		modem_binary_send_error(state, invalid_value);
		return;
	}
	if (result == verr_inv_access) {
		modem_binary_send_error(state, invalid_access);
		return;
	}
	if (result < 0)	{
		modem_binary_send_error(state, invalid_key);
		return;
	}

	// Fill	in response	data.
	response.header.response = MODEM_RSP_GET;
	response.header.result = result;
	memcpy(response.payload.get.var_name, var_name,	sizeof(response.payload.get.var_name));
	memcpy(response.payload.get.value, str_value, sizeof(response.payload.get.value));
	
	length = sizeof(struct header_response_s) +	sizeof(response.payload.get);
	LOG_INF("MODEM_RSP_GET:	result:	%d,	var_name: %s, value: %s", response.header.result,
		log_strdup(response.payload.get.var_name), log_strdup(response.payload.get.value));
	send_response_tty(state, &response,	length);
}

static void	binary_handler_push(struct modem_binary_state_s	*state,	struct modem_command_s *cmd, struct	modem_response_s *resp)
{
	uint32_t sequence;
	uint16_t length;
	struct modem_response_s	response = *resp;
	LOG_INF("MODEM_CMD_PUSH");
	
	// Get command data.
	char* report_name =	(char*)cmd->payload.push.report_name;
	LOG_INF("report_name: %s", log_strdup(report_name));

	int	param_count	= (int)cmd->payload.push.params_count;
	struct var_param_s params[param_count];
//	for	(int i=0; i<param_count; i++) {
		memcpy(&params[0].key[0], &cmd->payload.push.key[0][0],	sizeof(cmd->payload.push.key[0]));
		params[0].value	= cmd->payload.push.value[0];
		params[0].vlen = strlen((char*)cmd->payload.push.value[0])+1;
		LOG_INF("key: %s, value: %s", log_strdup(&params[0].key), log_strdup(params[0].value));
		LOG_INF("params[%d].vlen: %d", 0, params[0].vlen);
//	}
	int64_t	realtime = k_uptime_get(); // in LTE +	base_time;
	
	int	ret;
	
	ret	= lora_push(params[0].key, params[0].value);

	if (ret	< 0)
	{
		LOG_INF("sigconfig_push	fail");
		modem_binary_send_error(state, verr_inv_access);
		return;
	}

	// Fill	in response	data.
	response.header.response = MODEM_RSP_PUSH;
	response.header.result = 0;
	response.payload.push.mark = 0;

	length = sizeof(struct header_response_s) +	sizeof(response.payload.push);
	LOG_INF("MODEM_RSP_PUSH: result: %d, mark: %d",	response.header.result,	response.payload.push.mark);
	send_response_tty(state, &response,	length);
}
#if	0
static void	binary_handler_udp_resolve(struct modem_binary_state_s *state, struct modem_command_s *cmd,	struct modem_response_s	*resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;
	struct addrinfo	*resolve_result;
	struct addrinfo	hints =	{
		.ai_family = AF_INET,
		.ai_socktype = SOCK_DGRAM
	};

	LOG_INF("MODEM_CMD_UDP_RESOLVE");
	response.header.response = MODEM_RSP_UDP_RESOLVE;
	response.header.result = getaddrinfo(cmd->payload.resolve.name,	NULL, &hints, &resolve_result);

	if (response.header.result == 0) {
		memcpy(&response.payload.resolve.ip, &(((struct	sockaddr_in	*)resolve_result->ai_addr)->sin_addr.s_addr), sizeof(response.payload.resolve.ip));
	}
	length = sizeof(struct header_response_s) +	sizeof(response.payload.resolve);
	send_response_tty(state, &response,	length);
}

static void	binary_handler_tcp_resolve(struct modem_binary_state_s *state, struct modem_command_s *cmd,	struct modem_response_s	*resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;
	struct addrinfo	*resolve_result;
	struct addrinfo	hints =	{
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

	LOG_INF("MODEM_CMD_TCP_RESOLVE");
	response.header.response = MODEM_RSP_TCP_RESOLVE;
	response.header.result = getaddrinfo(cmd->payload.resolve.name,	NULL, &hints, &resolve_result);

	if (response.header.result == 0) {
		memcpy(&response.payload.resolve.ip, &(((struct	sockaddr_in	*)resolve_result->ai_addr)->sin_addr.s_addr), sizeof(response.payload.resolve.ip));
	}
	length = sizeof(struct header_response_s) +	sizeof(response.payload.resolve);
	send_response_tty(state, &response,	length);
}

static void	binary_handler_udp_open(struct modem_binary_state_s	*state,	struct modem_command_s *cmd, struct	modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;
	int	result;

	LOG_INF("MODEM_CMD_OPEN");
	result = client_connect(&(state->s), cmd->payload.udp_open.ip, cmd->payload.udp_open.port, IPPROTO_UDP,	0);

	response.header.response = MODEM_RSP_UDP_OPEN;
	response.header.result = result	< 0	? result : 0;
	if (result >= 0) {
		response.payload.open.socket = result;
	}
	else {
		response.payload.open.socket = -1;
	}

	length = sizeof(struct header_response_s) +	sizeof(response.payload.open);
	LOG_INF("Sending response");
	send_response_tty(state, &response,	length);
}

static void	binary_handler_udp_send(struct modem_binary_state_s	*state,	struct modem_command_s *cmd, struct	modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;

#if	0
	if (!check_command_size(cmd, sizeof(cmd->payload.send))) {
		modem_send_error(102);
		return 102;
	}
#endif
	LOG_INF("MODEM_CMD_UDP_SEND");
	response.payload.send.socket = cmd->payload.send.socket;
	if (cmd->payload.send.socket >=	0 && cmd->payload.send.socket <	MAX_MODEM_SOCKET) {
		int	sock = state->s.sock_state[cmd->payload.send.socket].sock;
		if (sock > 0) {
			int	length = cmd->payload.send.length;
#if	defined(FIFO)
			if (length < sizeof(send_items[0].data)) {
				int	i;
				struct send_item_s *item = NULL;
				response.payload.send.q_index =	-1;
				k_sem_take(&sem_uart, K_FOREVER);
				for	(i = 0 ; (i	< sizeof(send_items)/sizeof(send_items[0]))	&& (item ==	NULL) ;	i++) {
					if (send_items[i].in_use ==	0) {
						item = &send_items[i];
						response.payload.send.q_index =	i;
					}
				}
				if (item) {
					item->sock = sock;
					memcpy(item->data, cmd->payload.send.data, length);
					item->length = length;
					item->in_use = 1;
					k_fifo_put(&send_fifo, item);
					k_sem_give(&sem_uart);

					response.header.result = 0;
					response.payload.send.length = length;
				}
				else {
					k_sem_give(&sem_uart);
					response.header.result = -ENOMEM;
					response.payload.send.length = 0;
				}
			}
#else
			int	result = client_send(&(state->s), sock,	 cmd->payload.send.data, length);
			if (result == 0) {
				response.payload.send.length = length;
			}
			else {
				response.payload.send.length = 0;
			}
			response.header.result = result;
#endif
		}
		else {
			response.header.result = -EINVAL;
			response.payload.send.length = 0;
		}
	}
	else {
		response.header.result = -EINVAL;
		response.payload.send.length = 0;
	}

	response.header.response = MODEM_RSP_UDP_SEND,

	length = sizeof(struct header_response_s) +	sizeof(response.payload.send);
	send_response_tty(state, &response,	length);
}
#endif
static void	binary_handler_recv(struct modem_binary_state_s	*state,	struct modem_command_s *cmd, struct	modem_response_s *resp)
{
	//struct modem_response_s response = *resp;
	//uint16_t length;

	LOG_INF("MODEM_CMD_RECV");
}
#if	0
static void	binary_handler_udp_close(struct	modem_binary_state_s *state, struct	modem_command_s	*cmd, struct modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;

	LOG_INF("MODEM_CMD_CLOSE");
	
	response.header.result = client_disconnect(&(state->s),	cmd->payload.close.socket);
	if (response.header.result < 0)	{
		LOG_ERR("udp socket	failed to close.");
	}

	response.header.response = MODEM_RSP_UDP_CLOSE,

	length = sizeof(struct header_response_s);
	send_response_tty(state, &response,	length);
}

static void	binary_handler_tcp_open(struct modem_binary_state_s	*state,	struct modem_command_s *cmd, struct	modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;
	int	result;

	LOG_INF("MODEM_CMD_TCP_OPEN");
	result = client_connect(&(state->s), cmd->payload.tcp_open.ip, cmd->payload.tcp_open.port, IPPROTO_TCP,	cmd->payload.tcp_open.sectag);

	response.header.response = MODEM_RSP_TCP_OPEN;
	response.header.result = result	< 0	? result : 0;
	if (result >= 0) {
		response.payload.open.socket = result;
	}
	else {
		response.payload.open.socket = -1;
	}

	length = sizeof(struct header_response_s) +	sizeof(response.payload.open);
	LOG_INF("Sending response");
	send_response_tty(state, &response,	length);
}

static void	binary_handler_tcp_send(struct modem_binary_state_s	*state,	struct modem_command_s *cmd, struct	modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;

#if	0
	if (!check_command_size(cmd, sizeof(cmd->payload.send))) {
		modem_send_error(102);
		return 102;
	}
#endif
	LOG_INF("MODEM_CMD_TCP_SEND");
	response.payload.send.socket = cmd->payload.send.socket;
	if (cmd->payload.send.socket >=	0 && cmd->payload.send.socket <	MAX_MODEM_SOCKET) {
		int	sock = state->s.sock_state[cmd->payload.send.socket].sock;
		if (sock > 0) {
			int	length = cmd->payload.send.length;
#if	defined(FIFO)
			if (length < sizeof(send_items[0].data)) {
				int	i;
				struct send_item_s *item = NULL;
				response.payload.send.q_index =	-1;
				k_sem_take(&sem_uart, K_FOREVER);
				for	(i = 0 ; (i	< sizeof(send_items)/sizeof(send_items[0]))	&& (item ==	NULL) ;	i++) {
					if (send_items[i].in_use ==	0) {
						item = &send_items[i];
						response.payload.send.q_index =	i;
					}
				}
				if (item) {
					item->sock = sock;
					memcpy(item->data, cmd->payload.send.data, length);
					item->length = length;
					item->in_use = 1;
					k_fifo_put(&send_fifo, item);
					k_sem_give(&sem_uart);

					response.header.result = 0;
					response.payload.send.length = length;
				}
				else {
					k_sem_give(&sem_uart);
					response.header.result = -ENOMEM;
					response.payload.send.length = 0;
				}
			}
#else
			int	result = client_send(&(state->s), sock,	 cmd->payload.send.data, length);
			if (result == 0) {
				response.payload.send.length = length;
			}
			else {
				response.payload.send.length = 0;
			}
			response.header.result = result;
#endif
		}
		else {
			response.header.result = -EINVAL;
			response.payload.send.length = 0;
		}
	}
	else {
		response.header.result = -EINVAL;
		response.payload.send.length = 0;
	}

	response.header.response = MODEM_RSP_TCP_SEND,

	length = sizeof(struct header_response_s) +	sizeof(response.payload.send);
	send_response_tty(state, &response,	length);
}

static void	binary_handler_tcp_close(struct	modem_binary_state_s *state, struct	modem_command_s	*cmd, struct modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;

	LOG_INF("MODEM_CMD_TCP_CLOSE");
	
	response.header.result = client_disconnect(&(state->s),	cmd->payload.close.socket);
	if (response.header.result < 0)	{
		LOG_ERR("tcp socket	failed to close.");
	}

	response.header.response = MODEM_RSP_TCP_CLOSE,

	length = sizeof(struct header_response_s);
	send_response_tty(state, &response,	length);
}

static void	binary_handler_http_get(struct modem_binary_state_s	*state,	struct modem_command_s *cmd, struct	modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;
	uint8_t	buffer[sizeof(struct http_get_s)];
	int	len;
	
	LOG_INF("MODEM_CMD_HTTP_GET");
	
	// Fill	in header info
	if (cmd->payload.http_get.header.optional_headers[0] !=	'\0') {
		len	= snprintf(buffer, sizeof(struct http_get_s), HTTP_HEADER_TEMPLATE_OPTIONS,
				"GET", (const char *)cmd->payload.http_get.header.uri,
				(const char	*)cmd->payload.http_get.header.host,
				cmd->payload.http_get.header.connection	? "keep-alive" : "close",
				(const char	*)cmd->payload.http_get.header.optional_headers);
	}
	else {
		len	= snprintf(buffer, sizeof(struct http_get_s), HTTP_HEADER_TEMPLATE,
				"GET", (const char *)cmd->payload.http_get.header.uri,
				(const char	*)cmd->payload.http_get.header.host,
				cmd->payload.http_get.header.connection	? "keep-alive" : "close");
	}
	
	response.header.result = client_send(&(state->s), cmd->payload.http_get.socket,	buffer,	len);
	response.header.response = MODEM_RSP_HTTP_GET;
	length = sizeof(struct header_response_s);
	send_response_tty(state, &response,	length);
}

static void	binary_handler_http_put(struct modem_binary_state_s	*state,	struct modem_command_s *cmd, struct	modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;
	uint8_t	buffer[sizeof(struct http_put_s)];
	int	len;

	LOG_INF("MODEM_CMD_HTTP_PUT");
	
	// Fill	in header info
	if (cmd->payload.http_put.header.optional_headers[0] !=	'\0') {
		len	= snprintf(buffer, sizeof(struct http_put_s), HTTP_HEADER_TEMPLATE_OPTIONS,
				"PUT", (const char *)cmd->payload.http_put.header.uri,
				(const char	*)cmd->payload.http_put.header.host,
				cmd->payload.http_put.header.connection	? "keep-alive" : "close",
				(const char	*)cmd->payload.http_put.header.optional_headers);
	}
	else {
		len	= snprintf(buffer, sizeof(struct http_put_s), HTTP_HEADER_TEMPLATE,
				"PUT", (const char *)cmd->payload.http_put.header.uri,
				(const char	*)cmd->payload.http_put.header.host,
				cmd->payload.http_put.header.connection	? "keep-alive" : "close");
	}
	
	// Fill	in message
	strcat((char *)buffer, (const char *)cmd->payload.http_put.message);
	
	response.header.result = client_send(&(state->s), cmd->payload.http_put.socket,	buffer,	len);
	response.header.response = MODEM_RSP_HTTP_PUT;
	length = sizeof(struct header_response_s);
	send_response_tty(state, &response,	length);
}

static void	binary_handler_http_post(struct	modem_binary_state_s *state, struct	modem_command_s	*cmd, struct modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;
	uint8_t	buffer[sizeof(struct http_put_s)];
	int	len;

	LOG_INF("MODEM_CMD_HTTP_POST");
	
	// Fill	in header info
	if (cmd->payload.http_post.header.optional_headers[0] != '\0') {
		len	= snprintf(buffer, sizeof(struct http_post_s), HTTP_HEADER_TEMPLATE_OPTIONS,
				"POST",	(const char	*)cmd->payload.http_post.header.uri,
				(const char	*)cmd->payload.http_post.header.host,
				cmd->payload.http_post.header.connection ? "keep-alive"	: "close",
				(const char	*)cmd->payload.http_post.header.optional_headers);
	}
	else {
		len	= snprintf(buffer, sizeof(struct http_post_s), HTTP_HEADER_TEMPLATE,
				"POST",	(const char	*)cmd->payload.http_post.header.uri,
				(const char	*)cmd->payload.http_post.header.host,
				cmd->payload.http_post.header.connection ? "keep-alive"	: "close");
	}
	
	// Fill	in message
	strcat((char *)buffer, (const char *)cmd->payload.http_post.message);
	
	response.header.result = client_send(&(state->s), cmd->payload.http_post.socket, buffer, len);
	response.header.response = MODEM_RSP_HTTP_POST;
	length = sizeof(struct header_response_s);
	send_response_tty(state, &response,	length);
}

static void	binary_handler_http_delete(struct modem_binary_state_s *state, struct modem_command_s *cmd,	struct modem_response_s	*resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;
	uint8_t	buffer[sizeof(struct http_put_s)];
	int	len;

	LOG_INF("MODEM_CMD_HTTP_DELETE");
	
	// Fill	in header info
	if (cmd->payload.http_delete.header.optional_headers[0]	!= '\0') {
		len	= snprintf(buffer, sizeof(struct http_delete_s), HTTP_HEADER_TEMPLATE_OPTIONS,
				"DELETE", (const char *)cmd->payload.http_delete.header.uri,
				(const char	*)cmd->payload.http_delete.header.host,
				cmd->payload.http_delete.header.connection ? "keep-alive" :	"close",
				(const char	*)cmd->payload.http_delete.header.optional_headers);
	}
	else {
		len	= snprintf(buffer, sizeof(struct http_delete_s), HTTP_HEADER_TEMPLATE,
				"DELETE", (const char *)cmd->payload.http_delete.header.uri,
				(const char	*)cmd->payload.http_delete.header.host,
				cmd->payload.http_delete.header.connection ? "keep-alive" :	"close");
	}
	
	response.header.result = client_send(&(state->s), cmd->payload.http_delete.socket, buffer, len);
	response.header.response = MODEM_RSP_HTTP_DELETE;
	length = sizeof(struct header_response_s);
	send_response_tty(state, &response,	length);
}

static void	binary_handler_http_patch(struct modem_binary_state_s *state, struct modem_command_s *cmd, struct modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;
	uint8_t	buffer[sizeof(struct http_put_s)];
	int	len;

	LOG_INF("MODEM_CMD_HTTP_PATCH");
	
	// Fill	in header info
	if (cmd->payload.http_patch.header.optional_headers[0] != '\0')	{
		len	= snprintf(buffer, sizeof(struct http_patch_s),	HTTP_HEADER_TEMPLATE_OPTIONS,
				"PATCH", (const	char *)cmd->payload.http_patch.header.uri,
				(const char	*)cmd->payload.http_patch.header.host,
				cmd->payload.http_patch.header.connection ?	"keep-alive" : "close",
				(const char	*)cmd->payload.http_patch.header.optional_headers);
	}
	else {
		len	= snprintf(buffer, sizeof(struct http_patch_s),	HTTP_HEADER_TEMPLATE,
				"PATCH", (const	char *)cmd->payload.http_patch.header.uri,
				(const char	*)cmd->payload.http_patch.header.host,
				cmd->payload.http_patch.header.connection ?	"keep-alive" : "close");
	}
	
	// Fill	in message
	strcat((char *)buffer, (const char *)cmd->payload.http_patch.message);
	
	response.header.result = client_send(&(state->s), cmd->payload.http_patch.socket, buffer, len);
	response.header.response = MODEM_RSP_HTTP_PATCH;
	length = sizeof(struct header_response_s);
	send_response_tty(state, &response,	length);
}

static void	binary_handler_close_all_sockets(struct	modem_binary_state_s *state, struct	modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;

	LOG_INF("MODEM_CMD_CLOSE_ALL_SOCKETS");
	
	close_all_sockets(&(state->s));
	
	response.header.result = 0;
	response.header.response = MODEM_RSP_CLOSE_ALL_SOCKETS;

	length = sizeof(struct header_response_s);
	send_response_tty(state, &response,	length);
}

static void	binary_handler_socket_status(struct	modem_binary_state_s *state, struct	modem_command_s	*cmd, struct modem_response_s *resp)
{
	struct modem_response_s	response = *resp;
	uint16_t length;

	LOG_INF("MODEM_CMD_SOCKET_STATUS");
	response.header.response = MODEM_RSP_SOCKET_STATUS;
	response.header.result = 0;

	response.payload.sock_stat.socket =	cmd->payload.sock_stat.socket;
	response.payload.sock_stat.status =	socket_status(&(state->s), response.payload.sock_stat.socket);
	response.payload.sock_stat.ip_proto	= state->s.sock_state[response.payload.sock_stat.socket].ip_proto;
	
	length = sizeof(struct header_response_s) +	sizeof(response.payload.sock_stat);
	send_response_tty(state, &response,	length);
}
#endif
static void	send_response_tty(struct modem_binary_state_s *state, struct modem_response_s *response, uint16_t length)
{
	uint16_t crc = 0;

	state->s.uart_send(MODEM_START_BYTE, 1);
	state->s.uart_send((uint8_t*)&length, 2);
	state->s.uart_send((uint8_t*)response, length);
	state->s.uart_send((uint8_t*)&crc, 2);
}

static int check_command_size(int buffer_length, int payload_length)
{
	LOG_DBG("buffer_length[%d] <= payload_length[%d] + sizeof(struct header_command_s)[%d]?", buffer_length, payload_length, sizeof(struct header_command_s));
	k_sleep(K_MSEC(1000));
	if (buffer_length >	(payload_length	+ sizeof(struct	header_command_s)))	{
		return false;
	}
	return true;
}

void modem_binary_send_error(struct	modem_binary_state_s *s, char error)
{
	static struct header_response_s	response;
	int	length;
	struct modem_binary_state_s	*state = (struct modem_binary_state_s*)s;

	response.response =	MODEM_RSP_ERROR;
	response.result	= error;

	length = sizeof(struct header_response_s);
	send_response_tty(state, (struct modem_response_s*)&response, length);
}
#if	0
int	modem_binary_check_socket_receive(struct modem_state_s *s)
{
	int	length;
	int	off	= 0;
	int	bytes =	0;
	char received[MAX_REC_LEN];

	static struct modem_response_s response;

	struct modem_binary_state_s	*state = (struct modem_binary_state_s*)s;

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
			
			// Respond with	received data, if any.
			if (off	> 0) {
				k_sem_take(&sem_uart, K_FOREVER);
				state->s.sock_state[sindex].ts = k_uptime_get();
				k_sem_give(&sem_uart);
				response.header.response = MODEM_RSP_RECV;
				response.header.result = 0;
				memcpy(&response.payload.recv.data,	&received, sizeof(received));
				response.payload.recv.socket = sock;
				response.payload.recv.length = off;
#if	!defined(LINUX)
				LOG_DBG("received: %s\n", log_strdup(&received));
				k_sleep(1000);
#endif				
				length = sizeof(struct header_response_s) +	sizeof(response.payload.recv) -	sizeof(response.payload.recv.data) + off;
				send_response_tty(state, (struct modem_response_s*)&response, length);
			}
		}
	}
	
	return off;
}

int	modem_check_socket_send(struct modem_state_s *state)
{
#if	defined(FIFO)
	struct send_item_s	*send_data;

	k_sem_take(&sem_uart, K_FOREVER);
	send_data =	k_fifo_get(&send_fifo, K_NO_WAIT);
	k_sem_give(&sem_uart);
	if (send_data) {
		int	result = client_send(state,	send_data->sock, send_data->data, send_data->length);
		if (result == 0) {
			send_data->in_use =	0;
		}
		else {
			k_sem_take(&sem_uart, K_FOREVER);
			k_fifo_put(&send_fifo, send_data);
			k_sem_give(&sem_uart);
		}
	 }
#endif

	return 0;
}
#endif