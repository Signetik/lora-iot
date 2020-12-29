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
/*	   xxxx								*/
/* 1ff71400-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gc_gatt_config_service_uuid  = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71400, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/*
 * Characteristics UUID
 */
/* 1ff71401-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_app_skey_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71401, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 1ff71402-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_nwk_skey_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71402, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 1ff71403-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_app_eui_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71403, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 1ff71404-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_app_key_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71404, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 1ff71405-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_join_eui_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71405, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 1ff71406-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_dev_eui_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71406, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 1ff71407-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_lora_mode_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71407, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 11ff71408-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_save_configuration_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71408, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));


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
				 sizeof(var_lora_mode));
}

static ssize_t write_lora_mode(struct bt_conn *conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	uint8_t	*value = attr->user_data;

	if (offset + len > sizeof(var_lora_mode))	{
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

