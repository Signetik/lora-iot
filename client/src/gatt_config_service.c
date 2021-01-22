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

static ssize_t read_lora_auth
	(
	struct	bt_conn	*conn, 
	const struct bt_gatt_attr *attr,
	void *buf, uint16_t	len,
	uint16_t offset
	);

static ssize_t write_lora_auth
	(
	struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	const void *buf, 
	uint16_t len, 
	uint16_t offset,
	uint8_t	flags
	);

static ssize_t read_lora_class
	(
	struct	bt_conn	*conn, 
	const struct bt_gatt_attr *attr,
	void *buf, uint16_t	len,
	uint16_t offset
	);

static ssize_t write_lora_class
	(
	struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	const void *buf, 
	uint16_t len, 
	uint16_t offset,
	uint8_t	flags
	);

static ssize_t read_lora_adr
	(
	struct	bt_conn	*conn, 
	const struct bt_gatt_attr *attr,
	void *buf, uint16_t	len,
	uint16_t offset
	);

static ssize_t write_lora_adr
	(
	struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	const void *buf, 
	uint16_t len, 
	uint16_t offset,
	uint8_t	flags
	);

static ssize_t read_lora_datarate
	(
	struct	bt_conn	*conn, 
	const struct bt_gatt_attr *attr,
	void *buf, uint16_t	len,
	uint16_t offset
	);

static ssize_t write_lora_datarate
	(
	struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	const void *buf, 
	uint16_t len, 
	uint16_t offset,
	uint8_t	flags
	);

static ssize_t read_lora_chan_mask
	(
	struct	bt_conn	*conn, 
	const struct bt_gatt_attr *attr,
	void *buf, uint16_t	len,
	uint16_t offset
	);

static ssize_t write_lora_chan_mask
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
static struct bt_uuid_128 gatt_dev_eui_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71405, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 1ff71406-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_lora_auth_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71406, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 11ff71407-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_save_configuration_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71407, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 1ff71408-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_lora_class_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71408, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 1ff71409-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_lora_chan_mask_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff71409, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 1ff7140a-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_lora_datarate_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff7140a, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));

/* 1ff7140b-addc-49da-8bb2-a7026e65426d	*/
static struct bt_uuid_128 gatt_lora_adr_characteristic_uuid	 = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x1ff7140b, 0xaddc, 0x49da, 0x8bb2, 0xa7026e65426d));


BT_GATT_SERVICE_DEFINE(config_svc,
	BT_GATT_PRIMARY_SERVICE(&gc_gatt_config_service_uuid),
	BT_GATT_CHARACTERISTIC(&gatt_app_skey_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_app_skey, write_lora_app_skey,
				   &var_lora_app_skey),

	BT_GATT_CHARACTERISTIC(&gatt_nwk_skey_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_nwk_skey, write_lora_nwk_skey,
				   &var_lora_nwk_skey),

	BT_GATT_CHARACTERISTIC(&gatt_app_eui_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_app_eui, write_lora_app_eui,
				   &var_lora_app_eui),

	BT_GATT_CHARACTERISTIC(&gatt_app_key_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_app_key, write_lora_app_key,
				   &var_lora_app_key),

	BT_GATT_CHARACTERISTIC(&gatt_dev_eui_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_dev_eui, write_lora_dev_eui,
				   &var_lora_dev_eui),

	BT_GATT_CHARACTERISTIC(&gatt_lora_auth_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_auth, write_lora_auth,
				   &var_lora_auth),

	BT_GATT_CHARACTERISTIC(&gatt_lora_class_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_class,	write_lora_class,
				   &var_lora_class),

	BT_GATT_CHARACTERISTIC(&gatt_lora_chan_mask_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_chan_mask,	write_lora_chan_mask,
				   &var_lora_chan_mask),

	BT_GATT_CHARACTERISTIC(&gatt_lora_datarate_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_datarate, write_lora_datarate,
				   &var_lora_datarate),

	BT_GATT_CHARACTERISTIC(&gatt_lora_adr_characteristic_uuid.uuid,
				   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				   read_lora_adr, write_lora_adr,
				   &var_lora_adr),

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
	struct var_bin_s *value	= attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data,	value->maxsize); //var_lora_app_skey.maxsize);
}

static ssize_t write_lora_app_skey(struct bt_conn *conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	struct var_bin_s *value	= attr->user_data;

	if (offset + len > value->maxsize)	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	printk("lora app skey len:%d\n", len);
	memcpy(value->data + offset,	buf,	len);

	return len;
}

/* ABP network session key	*/
static ssize_t read_lora_nwk_skey(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	struct var_bin_s *value	= attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data,	value->maxsize);
}

static ssize_t write_lora_nwk_skey(struct bt_conn *conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	struct var_bin_s *value	= attr->user_data;

	if (offset + len >value->maxsize)	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	printk("lora nwk skey len:%d\n",	len);
	memcpy(value->data + offset,	buf,	len);

	return len;
}

/* ABP application eui */
static ssize_t read_lora_app_eui(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	struct var_bin_s *value	= attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data,	value->maxsize);
}

static ssize_t write_lora_app_eui(struct	bt_conn	*conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	struct var_bin_s *value	= attr->user_data;

	if (offset + len > value->maxsize)	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	printk("lora app eui len:%d\n",	len);
	memcpy(value->data	+	offset,	buf,	len);

	return len;
}

/* OTAA	application	key	*/
static ssize_t read_lora_app_key(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	struct var_bin_s *value	= attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data,	value->maxsize);
}

static ssize_t write_lora_app_key(struct	bt_conn	*conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	struct var_bin_s	*value = attr->user_data;

	if (offset + len > value->maxsize)	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	printk("lora app key len:%d\n",	len);
	memcpy(value->data +	offset,	buf,	len);

	return len;
}


/* OTAA	Dev	EUI	*/
static ssize_t read_lora_dev_eui(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	struct var_bin_s *value	= attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data,	value->maxsize);
}

static ssize_t write_lora_dev_eui(struct	bt_conn	*conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	struct var_bin_s	*value = attr->user_data;

	if (offset + len > value->maxsize)	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	printk("lora dev eui len:%d\n",	len);
	memcpy(value->data +	offset,	buf,	len);

	return len;
}

/* Radio Channel mask	*/
static ssize_t read_lora_chan_mask(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	struct var_bin_s *value	= attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data,	value->maxsize);
}

static ssize_t write_lora_chan_mask(struct	bt_conn	*conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	struct var_bin_s	*value = attr->user_data;

	if (offset + len > value->maxsize)	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	printk("lora chan mask len:%d\n", len);
	memcpy(value->data +	offset,	buf,	len);

	return len;
}

/* LoRa	Mode */
static ssize_t read_lora_auth(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	struct var_str_s *value	= attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data,	value->size);
}

static ssize_t write_lora_auth(struct bt_conn *conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	struct var_str_s	*value = attr->user_data;

	if (offset + len > value->size)	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	printk("lora auth len:%d\n", len);
	memcpy(value->data +	offset,	buf,	len);

	return len;
}

/* LoRa	class */
static ssize_t read_lora_class(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	struct var_str_s *value	=	attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data,	value->size);
}

static ssize_t write_lora_class(struct bt_conn *conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	struct var_str_s	*value = attr->user_data;

	if (offset + len > value->size)	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	printk("lora class len:%d\n", len);
	memcpy(value->data +	offset,	buf,	len);

	return len;
}

/* LoRa	data rate */
static ssize_t read_lora_datarate(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	const char *value =	attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(var_lora_datarate));
}

static ssize_t write_lora_datarate(struct bt_conn *conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	uint8_t	*value = attr->user_data;

	if (offset + len > sizeof(var_lora_datarate))	{
	printk("ERR	lora datarate len:%d, size:%d\n", len, sizeof(var_lora_datarate));
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	printk("lora datarate len:%d\n", len);
	memcpy(value +	offset,	buf,	len);

	return len;
}

/* LoRa	adaptive data rate */
static ssize_t read_lora_adr(struct	bt_conn	*conn, const struct	bt_gatt_attr *attr,
			   void	*buf, uint16_t len,	uint16_t offset)
{
	uint8_t	*value =	attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(var_lora_adr));
}

static ssize_t write_lora_adr(struct bt_conn *conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	uint8_t	*value = attr->user_data;

	if (offset + len > sizeof(var_lora_adr))	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	printk("lora adr len:%d\n",	len);
	memcpy(value +	offset,	buf,	len);

	return len;
}




static ssize_t save_configuration_characteristic(struct	bt_conn	*conn,	const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t	offset,
			uint8_t	flags)
{
	// write to	NVS	here.
	save_vars_config();

	return len;
}

