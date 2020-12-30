/*============================================================================*
 *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                      SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#ifndef __MODEM_INTERFACE_API_H
#define __MODEM_INTERFACE_API_H

#if defined(LINUX)
#include <netinet/in.h>
#else
#include <net/net_ip.h>
#endif

typedef int (*uart_send_f)(uint8_t *data, int length);
typedef int (*uart_recv_f)(uint8_t *data, int length);

// https://developer.nordicsemi.com/nRF_Connect_SDK/doc/1.2.0/nrf/include/at_cmd.html?highlight=socket%20limit
#define MAX_MODEM_SOCKET 8  
#define MAX_REC_LEN 512

#define MAX_HOST_LEN 128
#define MAX_URI_LEN 64
#define MAX_OPTIONAL_HEADER_LEN 128
#define MAX_MESSAGE_LEN 256

#define HTTP_HEADER_TEMPLATE	\
	"%s /%s HTTP/1.1\r\n"	\
	"Host: %s\r\n"		\
	"Connection: %s\r\n"	\
	"\r\n"

#define HTTP_HEADER_TEMPLATE_OPTIONS	\
	"#%s /%s HTTP/1.1\r\n"		\
	"Host: %s\r\n"			\
	"Connection: %s\r\n"		\
	"%s"				\
	"\r\n"

struct http_request_header {
	uint8_t uri[MAX_URI_LEN];
	uint8_t host[MAX_HOST_LEN];
	uint8_t connection;
	
	// NOTE: Optional headers must be included as a single string and
	// include "\r\n" after each header to conform with the template above.
	uint8_t optional_headers[MAX_OPTIONAL_HEADER_LEN];
};

struct socket_state_s {
	int sock;
	uint8_t rx_buffer[MAX_REC_LEN];
	int rx_length;
	int tx_length;
	int64_t ts;
	int ip_proto;
};

struct modem_state_s {
	int sock;
	struct socket_state_s sock_state[MAX_MODEM_SOCKET];
	uart_send_f uart_send;
	uart_recv_f uart_recv;
};

struct modem_api_func {
	void (*init)(struct modem_state_s *state);
	int (*check_socket_receive)(struct modem_state_s *state);
	int (*process_byte)(struct modem_state_s *state, char data);
};

enum modem_result_code {
	success         = 0,
	invalid_command = 1,
	invalid_key     = 2,
	invalid_value   = 3,
	invalid_access  = 4,
	invalid_report  = 5,
};

void modem_api_init(struct modem_state_s *s, uart_send_f uart_send, uart_recv_f uart_recv);
int client_connect(struct modem_state_s *s,uint32_t ipaddr, uint16_t port,int proto, int sectag);
int client_send(struct modem_state_s *s, uint16_t socki, uint8_t *buffer, size_t length);
int client_disconnect(struct modem_state_s *s, int sock);
void close_all_sockets(struct modem_state_s *s);
uint8_t socket_status(struct modem_state_s *s, int sock);
int sectag_set(int sock, int sec_tag);

extern struct k_sem sem_uart;

#endif /* __MODEM_INTERFACE_API_H */
