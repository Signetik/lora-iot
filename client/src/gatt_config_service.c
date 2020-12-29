/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

/*
 * Module Includes.
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
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
#include "gatt_config_service.h"
#include <logging/log.h>

#include "signetik.h"
//#include "wdt_task.h"
#include "vars.h"

/*
 * Extract devicetree configuration.
 */


LOG_MODULE_REGISTER(gatt_config, CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);
/*
 * Module Defines
 */

/*
 * Forward Declarations
 */
static void	config_ccc_cfg_changed
	(
	const struct bt_gatt_attr *attr,
	uint16_t value
	);

static ssize_t read_lora_app_skey
	(
	struct bt_conn *conn, 
	const struct   bt_gatt_attr	*attr,
	void *buf, 
	uint16_t len,
	uint16_t offset
	);

static ssize_t write_lora_app_skey
	(
	struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	const void *buf, 
	uint16_t len, 
	uint16_t offset,
	uint8_t	flags
	);

static ssize_t read_lora_nwk_skey
	(
	struct	bt_conn	*conn, 
	const struct bt_gatt_attr *attr,
	void *buf, uint16_t	len,
	uint16_t offset
	);

static ssize_t write_lora_nwk_skey
	(
	struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	const void *buf, 
	uint16_t len, 
	uint16_t offset,
	uint8_t	flags
	);

static ssize_t read_lora_app_eui
	(
	struct	bt_conn	*conn, 
	const struct bt_gatt_attr *attr,
	void *buf, uint16_t	len,
	uint16_t offset
	);

static ssize_t write_lora_app_eui
	(
	struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	const void *buf, 
	uint16_t len, 
	uint16_t offset,
	uint8_t	flags
	);

static ssize_t read_lora_app_key
	(
	struct	bt_conn	*conn, 
	const struct bt_gatt_attr *attr,
	void *buf, uint16_t	len,
	uint16_t offset
	);

static ssize_t write_lora_app_key
	(
	struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	const void *buf, 
	uint16_t len, 
	uint16_t offset,
	uint8_t	flags
	);

static ssize_t read_lora_join_eui
	(
	struct	bt_conn	*conn, 
	const struct bt_gatt_attr *attr,
	void *buf, uint16_t	len,
	uint16_t offset
	);

static ssize_t write_lora_join_eui
	(
	struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	const void *buf, 
	uint16_t len, 
	uint16_t offset,
	uint8_t	flags
	);

static ssize_t read_lora_dev_eui
	(
	struct	bt_conn	*conn, 
	const struct bt_gatt_attr *attr,
	void *buf, uint16_t	len,
	uint16_t offset
	);

static ssize_t write_lora_dev_eui
	(
	struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	const void *buf, 
	uint16_t len, 
	uint16_t offset,
	uint8_t	flags
	);

static ssize_t read_lora_mode
	(
	struct	bt_conn	*conn, 
	const struct bt_gatt_attr *attr,
	void *buf, uint16_t	len,
	uint16_t offset
	);

static ssize_t write_lora_mode
	(
	struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	const void *buf, 
	uint16_t len, 
	uint16_t offset,
	uint8_t	flags
	);

static ssize_t save_configuration_characteristic
	(
	struct	bt_conn	*conn,	
	const struct bt_gatt_attr *attr,
	const void *buf,
	uint16_t len,
	uint16_t offset,
	uint8_t	flags
	);


/*
 * Module Variables.
 */

/* Custom Service Variables	*/
/* f2101400-b6e0-4617-810a-3934b3b43a39	*/
static struct bt_uuid_128 gc_gatt_config_service_uuid  = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xf2101400, 0xb6e0, 0x4617, 0x810a, 0x3934b3b43a39));

/*
 * Characteristics UUID
 */
/* f2101401-b6e0-4617-810a-3934b3b43a39	*/
static struct bt_uuid_128 gatt_app_skey_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xf2101401, 0xb6e0, 0x4617, 0x810a, 0x3934b3b43a39));

/* f2101402-b6e0-4617-810a-3934b3b43a39	*/
static struct bt_uuid_128 gatt_nwk_skey_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xf2101402, 0xb6e0, 0x4617, 0x810a, 0x3934b3b43a39));

/* f2101403-b6e0-4617-810a-3934b3b43a39	*/
static struct bt_uuid_128 gatt_app_eui_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xf2101403, 0xb6e0, 0x4617, 0x810a, 0x3934b3b43a39));

/* f2101404-b6e0-4617-810a-3934b3b43a39	*/
static struct bt_uuid_128 gatt_app_key_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xf2101404, 0xb6e0, 0x4617, 0x810a, 0x3934b3b43a39));

/* f2101405-b6e0-4617-810a-3934b3b43a39	*/
static struct bt_uuid_128 gatt_join_eui_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xf2101405, 0xb6e0, 0x4617, 0x810a, 0x3934b3b43a39));

/* f2101406-b6e0-4617-810a-3934b3b43a39	*/
static struct bt_uuid_128 gatt_dev_eui_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xf2101406, 0xb6e0, 0x4617, 0x810a, 0x3934b3b43a39));

/* f2101407-b6e0-4617-810a-3934b3b43a39	*/
static struct bt_uuid_128 gatt_lora_mode_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xf2101407, 0xb6e0, 0x4617, 0x810a, 0x3934b3b43a39));

/* f2101408-b6e0-4617-810a-3934b3b43a39	*/
static struct bt_uuid_128 gatt_save_configuration_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xf2101408, 0xb6e0, 0x4617, 0x810a, 0x3934b3b43a39));

/*
 * Characteristic Variables	(moved to vars.c soon)
 */
//static uint8_t		app_skey[APP_SKEY_MAX_SZ];
//static uint8_t		nwk_skey[NWK_SKEY_MAX_SZ];
//static uint8_t		app_eui[DEV_EUI_MAX_SZ];

//static uint8_t		app_key[APP_KEY_MAX_SZ];
//static uint8_t		join_eui[JOIN_EUI_MAX_SZ];
//static uint8_t		dev_eui[DEV_EUI_MAX_SZ];

static uint8_t		lora_mode;

BT_GATT_SERVICE_DEFINE(config_svc,
	BT_GATT_PRIMARY_SERVICE(&gc_gatt_config_service_uuid),
	BT_GATT_CHARACTERISTIC(&gatt_app_skey_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_app_skey, write_lora_app_skey,
				   &var_lora_app_skey.data),

	BT_GATT_CHARACTERISTIC(&gatt_nwk_skey_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_nwk_skey, write_lora_nwk_skey,
				   &var_lora_nwk_skey.data),

	BT_GATT_CHARACTERISTIC(&gatt_app_eui_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_app_eui, write_lora_app_eui,
				   &var_lora_app_eui.data),

	BT_GATT_CHARACTERISTIC(&gatt_app_key_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_app_key, write_lora_app_key,
				   &var_lora_app_key.data),

	BT_GATT_CHARACTERISTIC(&gatt_join_eui_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_join_eui, write_lora_join_eui,
				   &var_lora_join_eui.data),

	BT_GATT_CHARACTERISTIC(&gatt_dev_eui_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_dev_eui, write_lora_dev_eui,
				   &var_lora_dev_eui.data),

	BT_GATT_CHARACTERISTIC(&gatt_lora_mode_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_mode, write_lora_mode,
				   &var_lora_mode),

	BT_GATT_CHARACTERISTIC(&gatt_save_configuration_characteristic_uuid.uuid,
				   BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_WRITE,
				   NULL, save_configuration_characteristic,
				   NULL),

	BT_GATT_CCC(config_ccc_cfg_changed,
			BT_GATT_PERM_READ |	BT_GATT_PERM_WRITE)
);

/*
 * Constants
 */

/*
 * Functions
 */
static void	config_ccc_cfg_changed(const struct	bt_gatt_attr *attr,
					   uint16_t	value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value	== BT_GATT_CCC_NOTIFY);

	LOG_INF("Config	Notifications %s", notif_enabled ? "enabled" : "disabled");
}

/* ABP application session key	*/
static ssize_t read_lora_app_skey(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	const char *value =	attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(var_lora_app_skey.data));
}

static ssize_t write_lora_app_skey(struct bt_conn *conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	uint8_t	*value = attr->user_data;

	if (offset + len > sizeof(var_lora_app_skey.data))	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf,	len);

	return len;
}

/* ABP network session key	*/
static ssize_t read_lora_nwk_skey(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	const char *value =	attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(var_lora_nwk_skey.data));
}

static ssize_t write_lora_nwk_skey(struct bt_conn *conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	uint8_t	*value = attr->user_data;

	if (offset + len > sizeof(var_lora_nwk_skey.data))	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf,	len);

	return len;
}

/* ABP application eui */
static ssize_t read_lora_app_eui(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	const char *value =	attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(var_lora_app_eui.data));
}

static ssize_t write_lora_app_eui(struct	bt_conn	*conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	uint8_t	*value = attr->user_data;

	if (offset + len > sizeof(var_lora_app_eui.data))	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf,	len);

	return len;
}

/* OTAA	application	key	*/
static ssize_t read_lora_app_key(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	const char *value =	attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(var_lora_app_key.data));
}

static ssize_t write_lora_app_key(struct	bt_conn	*conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	uint8_t	*value = attr->user_data;

	if (offset + len > sizeof(var_lora_app_key.data))	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf,	len);

	return len;
}

/* OTAA	Join EUI */
static ssize_t read_lora_join_eui(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	const char *value =	attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(var_lora_join_eui.data));
}

static ssize_t write_lora_join_eui(struct bt_conn *conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	uint8_t	*value = attr->user_data;

	if (offset + len > sizeof(var_lora_join_eui.data))	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf,	len);

	return len;
}

/* OTAA	Dev	EUI	*/
static ssize_t read_lora_dev_eui(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	const char *value =	attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(var_lora_dev_eui.data));
}

static ssize_t write_lora_dev_eui(struct	bt_conn	*conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	uint8_t	*value = attr->user_data;

	if (offset + len > sizeof(var_lora_dev_eui.data))	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf,	len);

	return len;
}

/* LoRa	Mode */
static ssize_t read_lora_mode(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	const char *value =	attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(lora_mode));
}

static ssize_t write_lora_mode(struct bt_conn *conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	uint8_t	*value = attr->user_data;

	if (offset + len > sizeof(lora_mode))	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf,	len);

	return len;
}





static ssize_t save_configuration_characteristic(struct	bt_conn	*conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	// write to	NVS	here.
	return len;
}

