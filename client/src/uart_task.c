/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr.h>
#include <drivers/uart.h>
#include <console/tty.h>
#include <logging/log.h>
#include <drivers/gpio.h>
#include <net/socket.h>
#include <fcntl.h>

#include "signetik.h"
#include "sigconfig.h"
#include "vars.h"
#include "wdt_task.h"
#include "uart_task.h"

//#include "modem-api/modem_interface_api.h"

#if	defined(CONFIG_SENSOR_POLLING_METHOD_UART_BINARY)
#include "modem-api/modem_binary_interface_api.h"
#endif
#if	defined(CONFIG_SENSOR_POLLING_METHOD_UART_PUSH)
#include "modem-api/modem_ascii_interface_api.h"
#endif

#define	DEFAULT_UART_NODE DT_ALIAS(uart0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_UART_NODE,	okay),
		 "No default Uart Interface	specified in DT");
#define	DEFAULT_UART DT_LABEL(DEFAULT_UART_NODE)

LOG_MODULE_REGISTER(uart, CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

K_SEM_DEFINE(sem_tty, 0, 1);

//static void	process_bytes(struct modem_state_s *modem_state, char *data, int length);
static struct tty_serial tty;
static uint8_t uart_buffer[2048]; // The TTY code has a	bug. If	you	fill this up it	crashes!


union ms_s {
#if	defined(CONFIG_SENSOR_POLLING_METHOD_UART_BINARY)
	struct modem_binary_state_s	binary;
#endif
#if	defined(CONFIG_SENSOR_POLLING_METHOD_UART_PUSH)
	struct modem_ascii_state_s ascii;
#endif
} ms;

static struct modem_state_s	*modem_state = (struct modem_state_s*)&ms;
#if	defined(MODEM_FIFO)
static bool	modem_state_initialized	= false;
#endif

static struct modem_api_func* api =	NULL;

void uart_rx_thread(void *p1, void *p2,	void *p3)
{
	struct device *	uart;

	sigconfig_init();

	uart = device_get_binding(DEFAULT_UART);
	if (!uart) {
		printk("Cannot find	uart(%s)!\n", DEFAULT_UART);
		return;
	}

	tty_init(&tty, uart);
	tty_set_rx_timeout(&tty, 10);
	tty_set_rx_buf(&tty, uart_buffer, sizeof(uart_buffer));

	k_sem_give(&sem_tty);

//	modem_api_init(modem_state,	uart_send, NULL); // TODO: I need to review	the	NULL. (see next	TODO)

#if	!defined(TEST_CODE)
//	while (!LTE_wait_complete(250))	{
#endif
//		set_leds(false);
#if	!defined(TEST_CODE)
//	}
#endif

	// Register	with WDT.
	uint8_t	thread_id =	wdt_register_thread();

	while (1)
	{
		// Feed	WDT	(must use assigned thread ID).
		wdt_feed_watchdog(thread_id);

		char buffer[8];
		int	count;
//		static struct modem_api_func* new_api;

//		if (var_binary)	{
//			new_api	= modem_binary_api_get_func();
//		}
//		else {
//			new_api	= modem_ascii_api_get_func();
//		}
//		if (new_api	!= api)	{
//			api	= new_api;
//			api->init(modem_state);
//			uart_send("+notify,event:init,result:0,firmware:", 0);
//			uart_send(var_firmware.data, 0);
//			uart_send("\r\n", 0);
//		#if	defined(MODEM_FIFO)
//			modem_state_initialized	= true;
//		#endif
//		}

		count =	tty_read(&tty, buffer, sizeof(buffer));	// TODO: Down the road move	this read to a uart_recv callback
		if (count >	0) {
//			process_bytes(modem_state, buffer, count);
		}
		k_sleep(K_MSEC(1000));
	}
}

void uart_tx_thread(void *p1, void *p2,	void *p3)
{

	// Register	with WDT.
	uint8_t	thread_id =	wdt_register_thread();

	while (1)
	{
		// Feed	WDT	(must use assigned thread ID).
		wdt_feed_watchdog(thread_id);


//		if (api->check_socket_receive(modem_state) <= 0)	{
//		}
		k_sleep(K_MSEC(1000));

	}
}

#if	defined(MODEM_FIFO)
void uart_send_thread(void)
{
	while (!modem_state_initialized) {
		k_sleep(100);
	}

	// Register	with WDT.
	uint8_t	thread_id =	wdt_register_thread();

	while (1)
	{
		// Feed	WDT	(must use assigned thread ID).
		wdt_feed_watchdog(thread_id);

		modem_check_socket_send(&modem_state);
		k_sleep(1000);
	}
}
#endif

int	uart_send(uint8_t *buffer, int length)
{
	int	result;

	if (length == 0) {
		length = strlen(buffer);
	}

	k_sem_take(&sem_tty, K_FOREVER);
	result = tty_write(&tty, buffer, length);
	k_sem_give(&sem_tty);

	return result;
}

static void	process_bytes(struct modem_state_s *modem_state, char *data, int length)
{
	for	(int index = 0 ; index < length	; index++) {
//		api->process_byte(modem_state, data[index]);
	}

}


///	Create UART	threads/tasks.
#if	defined(MODEM_FIFO)
K_THREAD_DEFINE(uart_send_id, UART_STACKSIZE, uart_send_thread,	NULL, NULL,	NULL, UART_PRIORITY, 0,	K_NO_WAIT);
#endif

K_THREAD_STACK_DEFINE(uart_rx_stack_area, UART_STACKSIZE);
struct k_thread	uart_rx_thread_data;

K_THREAD_STACK_DEFINE(uart_tx_stack_area, UART_STACKSIZE);
struct k_thread	uart_tx_thread_data;

///	Start UART threads.
void uart_thread_start(void)
{
	/* k_tid_t my_tid =	*/
	k_thread_create(&uart_rx_thread_data, uart_rx_stack_area,
		K_THREAD_STACK_SIZEOF(uart_rx_stack_area),
		uart_rx_thread,
		NULL, NULL,	NULL,
		UART_PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&uart_tx_thread_data, uart_tx_stack_area,
		K_THREAD_STACK_SIZEOF(uart_tx_stack_area),
		uart_tx_thread,
		NULL, NULL,	NULL,
		UART_PRIORITY, 0, K_NO_WAIT);
}
