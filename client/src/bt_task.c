/*============================================================================*
 *         Copyright © 2019-2021 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                          SPDX-License-Identifier: Sigentik *
 *                                                                            *
 * Customer may modify, compile, assemble and convert this source code into   *
 * binary object or executable code for use on Signetk products purchased     *
 * from Signetik or its distributors.                                         *
 *                                                                            *
 * Customer may incorporate or embed an binary executable version of the      *
 * software into the Customer’s product(s), which incorporate a Signetik      *
 * product purchased from Signetik or its distributors. Customer may          *
 * manufacture, brand and distribute such Customer’s product(s) worldwide to  *
 * its End-Users.                                                             *
 *                                                                            *
 * This agreement must be formalized with Signetik before Customer enters     *
 * production and/or distributes products to Customer's End-Users             *
 *============================================================================*/

/*
 * Module Includes.
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <logging/log.h>

#include "signetik.h"
#include "wdt_task.h"
#include "bt_task.h"
#include "vars.h"

/*
 * Extract devicetree configuration.
 */


LOG_MODULE_REGISTER(bt_task,	CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

/*
 * Module Defines
 */



/*
 * Module Variables.
 */
static const struct	bt_data	ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
	BT_UUID_128_ENCODE(0x1ff71400, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d))
};

static void	connected(struct bt_conn *conn,	uint8_t	err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void	disconnected(struct	bt_conn	*conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
}

static struct bt_conn_cb conn_callbacks	= {
	.connected = connected,
	.disconnected =	disconnected,
};

static void	bt_ready(void)
{
	int	err;

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err	= bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising	failed to start	(err %d)\n", err);
		return;
	}

	printk("Advertising	successfully started\n");
}

static void	auth_passkey_display(struct	bt_conn	*conn, unsigned	int	passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey	for	%s:	%06u\n", addr, passkey);
}

static void	auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing	cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display =	{
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel	= auth_cancel,
};


/*
 * Public Functions
 */
void bt_thread(void	*p1, void	*p2, void *p3)
{
	int	err;
	uint8_t	thread_id;
	uint8_t	buf[CONFIG_BT_DEVICE_NAME_MAX];

	err	= bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	// create advertised device	name from device name and ID

	snprintf(buf, sizeof(buf), "%s-%s",	CONFIG_BT_DEVICE_NAME, var_devid.data );
	bt_set_name(buf);

	bt_ready();

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	// Register	with WDT.
	thread_id =	wdt_register_thread();

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work	so we do it	here
	 */
	while (1) 
	{
		// Feed	WDT	(must use assigned thread ID).
		wdt_feed_watchdog(thread_id);
		k_sleep(K_MSEC(1000));

	}
}

///	CreateBluetooth	thread/task.
K_THREAD_STACK_DEFINE(bt_stack_area, BT_STACKSIZE);
struct k_thread	bt_thread_data;

///	Start Bluetooth	thread.
void bt_thread_start(void)
{
	/* k_tid_t my_tid =	*/
	k_thread_create(&bt_thread_data, bt_stack_area,
		K_THREAD_STACK_SIZEOF(bt_stack_area),
		bt_thread,
		NULL, NULL,	NULL,
		BT_PRIORITY, 0,	K_NO_WAIT);
}

