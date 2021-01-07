/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#ifndef	__MODEM_BINARY_INTERFACE_API_SHARED_H
#define	__MODEM_BINARY_INTERFACE_API_SHARED_H

#include "modem_binary_interface_api.h"
#include "vars.h"

#define	MODEM_START_BYTE ">"

#define	MODEM_CMD_INFO		10
#define	MODEM_CMD_SET		20
#define	MODEM_CMD_GET		21
#define	MODEM_CMD_PUSH		22
#define	MODEM_CMD_UDP_OPEN	50
#define	MODEM_CMD_UDP_SEND	51
#define	MODEM_CMD_UDP_CLOSE	52
#define	MODEM_CMD_TCP_OPEN	53
#define	MODEM_CMD_TCP_SEND	54
#define	MODEM_CMD_TCP_CLOSE	55
#define	MODEM_CMD_RECV		56
#define	MODEM_CMD_UDP_RESOLVE	57
#define	MODEM_CMD_TCP_RESOLVE	58
#define	MODEM_CMD_HTTP_GET		60
#define	MODEM_CMD_HTTP_PUT		61
#define	MODEM_CMD_HTTP_POST		62
#define	MODEM_CMD_HTTP_DELETE	63
#define	MODEM_CMD_HTTP_PATCH	64
#define	MODEM_CMD_SOCKET_STATUS	80
#define	MODEM_CMD_CLOSE_ALL_SOCKETS	81
#define	MODEM_CMD_ERROR		255

#define	MODEM_RSP_INFO		MODEM_CMD_INFO
#define	MODEM_RSP_SET		MODEM_CMD_SET
#define	MODEM_RSP_GET		MODEM_CMD_GET
#define	MODEM_RSP_PUSH		MODEM_CMD_PUSH
#define	MODEM_RSP_UDP_OPEN	MODEM_CMD_UDP_OPEN
#define	MODEM_RSP_UDP_SEND	MODEM_CMD_UDP_SEND
#define	MODEM_RSP_UDP_CLOSE	MODEM_CMD_UDP_CLOSE
#define	MODEM_RSP_TCP_OPEN	MODEM_CMD_TCP_OPEN
#define	MODEM_RSP_TCP_SEND	MODEM_CMD_TCP_SEND
#define	MODEM_RSP_TCP_CLOSE	MODEM_CMD_TCP_CLOSE
#define	MODEM_RSP_RECV		MODEM_CMD_RECV
#define	MODEM_RSP_UDP_RESOLVE	MODEM_CMD_UDP_RESOLVE
#define	MODEM_RSP_TCP_RESOLVE	MODEM_CMD_TCP_RESOLVE
#define	MODEM_RSP_HTTP_GET		MODEM_CMD_HTTP_GET
#define	MODEM_RSP_HTTP_PUT		MODEM_CMD_HTTP_PUT
#define	MODEM_RSP_HTTP_POST		MODEM_CMD_HTTP_POST
#define	MODEM_RSP_HTTP_DELETE	MODEM_CMD_HTTP_DELETE
#define	MODEM_RSP_HTTP_PATCH	MODEM_CMD_HTTP_PATCH
#define	MODEM_RSP_SOCKET_STATUS	MODEM_CMD_SOCKET_STATUS
#define	MODEM_RSP_CLOSE_ALL_SOCKETS	MODEM_CMD_CLOSE_ALL_SOCKETS
#define	MODEM_RSP_ERROR		MODEM_CMD_ERROR	

struct __attribute__((__packed__)) modem_command_s {
	struct __attribute__((__packed__)) header_command_s	{
		uint8_t	command;	///	Binary representation of modem commands.
		uint32_t token;		///	Incremented	token of current command.
	} header;
	union payload_command_s	{
		struct __attribute__((__packed__)) info_s {
		} info;
		
		struct __attribute__((__packed__)) set_s {
			uint8_t	var_name[MAX_KEY_LEN];
			uint8_t	value[MAX_VAL_LEN];
		} set;
		struct __attribute__((__packed__)) get_s {
			uint8_t	var_name[MAX_KEY_LEN];
		} get;
		struct __attribute__((__packed__)) push_s {
			uint8_t	report_name[VAR_REPORT_NAME_SIZE];
			uint8_t	key[VAR_MAX_FIELDS][MAX_KEY_LEN];
			uint8_t	value[VAR_MAX_FIELDS][MAX_FIELD_VAL_LEN];
			uint8_t	params_count;
		} push;
		
		struct __attribute__((__packed__)) resolve_s {
			uint8_t	name[MAX_HOST_LEN];
		} resolve;
		struct __attribute__((__packed__)) udp_open_s {
			uint32_t ip;
			uint16_t port;
		} udp_open;
		struct __attribute__((__packed__)) tcp_open_s {
			uint32_t ip;
			uint16_t port;
			int	sectag;
		} tcp_open;
		struct __attribute__((__packed__)) send_s {
			uint32_t socket;
			uint16_t length;
			uint8_t	data[512];
		} send;
		struct __attribute__((__packed__)) close_s {
			uint32_t socket;
		} close;
		struct __attribute__((__packed__)) sock_stat_s {
			uint8_t	socket;
		} sock_stat;
		struct __attribute__((__packed__)) recv_s {
			uint32_t socket;
			uint16_t length;
		} recv;
		
		struct __attribute__((__packed__)) http_get_s {
			uint32_t socket;
			struct http_request_header header;
		} http_get;
		struct __attribute__((__packed__)) http_put_s {
			uint32_t socket;
			struct http_request_header header;
			uint8_t	message[MAX_MESSAGE_LEN];
		} http_put;
		struct __attribute__((__packed__)) http_post_s {
			uint32_t socket;
			struct http_request_header header;
			uint8_t	message[MAX_MESSAGE_LEN];
		} http_post;
		struct __attribute__((__packed__)) http_delete_s {
			uint32_t socket;
			struct http_request_header header;
		} http_delete;
		struct __attribute__((__packed__)) http_patch_s	{
			uint32_t socket;
			struct http_request_header header;
			uint8_t	message[MAX_MESSAGE_LEN];
		} http_patch;

	} payload;
};

struct __attribute__((__packed__)) modem_response_s	{
	struct __attribute__((__packed__)) header_response_s {
		uint8_t	response;	///	Command	being responded	to.
		int16_t	result;		///	Result of command.
		uint32_t token;		///	Incremented	token of current command.
	} header;
	union payload_response_s {
		struct __attribute__((__packed__)) info_response_s {
			uint8_t	imei[32];
			uint32_t devid;
			uint8_t	state;
		} info;
		
		struct __attribute__((__packed__)) get_response_s {
			uint8_t	var_name[MAX_KEY_LEN];
			uint8_t	value[MAX_VAL_LEN];
		} get;
		struct __attribute__((__packed__)) push_response_s {
			uint32_t mark;
		} push;
		
		struct __attribute__((__packed__)) resolve_response_s {
			uint32_t ip;
		} resolve;
		struct __attribute__((__packed__)) open_response_s {
			uint32_t socket;
		} open;
		struct __attribute__((__packed__)) send_response_s {
			uint32_t socket;
			uint16_t length;
			uint8_t	q_index;
		} send;
		struct __attribute__((__packed__)) recv_response_s {
			uint32_t socket;
			uint16_t length;
			uint8_t	data[MAX_REC_LEN];
		} recv;
		
		struct __attribute__((__packed__)) sock_stat_response_s	{
			uint32_t socket;
			uint8_t	status;
			int	ip_proto;
		} sock_stat;

	} payload;
};

int	modem_binary_process_byte(struct modem_state_s *s, char	data);
void modem_binary_send_error(struct	modem_binary_state_s *state, char error);
int	modem_binary_parse(struct modem_state_s	*s,	uint8_t	*data, int data_length);

#endif /* __MODEM_BINARY_INTERFACE_API_SHARED_H	*/
