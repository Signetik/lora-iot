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

#if	!defined(LINUX)
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <drivers/flash.h>
#include <fs/nvs.h>
#include <storage/flash_map.h>
#include <power/reboot.h>
#include <logging/log.h>
//#include <nrf_socket.h>
#else
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#endif

#include "vars.h"

LOG_MODULE_REGISTER(vars, CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

void* sigconfig_set_report_names(void *value);
void* cacert_write(void* cert);
void* privcert_write(void* cert);
void* privkey_write(void* cert);
void* cacert_read(void);
void* privcert_read(void);
void* privkey_read(void);
void* fota_start(void *var);
void* fotasub_start(void *var);
void* imsi_read(void);
void* iccid_read(void);
void* fotasuback_set(void* arg);
void* at_set(void* arg);
void* reboot(void*);

///	Sensor API
void* sensor_enable(void);

///	GPS	API

static int vars_flash_init(void);
static int vars_flash_read(int id, void	*buffer, int buf_sz);
static void* flash_save(void* data);
static void* flash_load(void);
static void* flash_clear(void);

#define	VAR_STR_CREATE(_name_,_size_,_default_)	\
	uint8_t	_name_[_size_]	= _default_; \
	struct var_str_s var_##_name_ =	{ \
		_name_,	sizeof(_default_), sizeof(_name_) \
	};

#define	VAR_BIN_PROTECT(...) __VA_ARGS__ 
#define	VAR_BIN_CREATE(_name_,_minsize_,_maxsize_,_default_)	\
	uint8_t	_name_[_maxsize_]	= _default_; \
	struct var_bin_s var_##_name_ =	{ \
		_name_,	_maxsize_, _minsize_, _maxsize_	\
	};

bool var_echo =	true;

// Variables
VAR_STR_CREATE(devid,		 17, "");
VAR_STR_CREATE(devtype,		 32, "Default");
VAR_STR_CREATE(imei,		 32, "");
VAR_STR_CREATE(user,		 80, "user");
VAR_STR_CREATE(pw,			257, "");
VAR_STR_CREATE(firmware,	 32, "v0.0.0");
VAR_STR_CREATE(mfirmware,	 32, "v0.0.0");
VAR_STR_CREATE(proto,		  8, "lora");
VAR_STR_CREATE(current_proto, 8, "none");
VAR_STR_CREATE(sensor, 32, "temp_humid");
VAR_STR_CREATE(sensor_board, 32, "THA");

VAR_STR_CREATE(host,  65, "iot.aws.signetik.com");
uint16_t var_port =	5715;
VAR_STR_CREATE(apikey,	65,	"");
VAR_STR_CREATE(uri,	 65, "*");

uint32_t var_device_id	= 0;
uint8_t	 var_battery	= 0;
uint32_t var_queue_mark	= 0;
uint32_t var_push_mark	= 0;
uint32_t var_rsrp		= 0;

bool var_connected	= false;
bool var_connect	= true;
bool var_auto_connect	= true;
bool var_enabled	= false;
bool var_sensor_poll	= false;
bool var_binary		= false;
bool var_leds		= true;

// LoRa	Vars
bool var_lora_adr =	false;
uint8_t	var_lora_datarate	= 1;
uint16_t var_lora_rxdelay1	= 1000;
uint16_t var_lora_rxdelay2	= 2000;
VAR_STR_CREATE(lora_class, 2, "C");
VAR_STR_CREATE(lora_auth, 5, "ABP");
VAR_BIN_CREATE(lora_app_skey, 16, 16, VAR_BIN_PROTECT({0x6F, 0x7B, 0x80, 0xF7, 0xE4, 0xD0, 0xB9, 0xE5, 0x1F, 0xE9, 0xF8, 0x97, 0x64, 0x15, 0xBD, 0xD7}));
VAR_BIN_CREATE(lora_nwk_skey, 16, 16, VAR_BIN_PROTECT({0x0B, 0x30, 0x52, 0x51, 0xA6, 0x0C, 0x52, 0x11, 0x72, 0x32, 0x85, 0xD1, 0xFB, 0x2E, 0xF8, 0x39}));
VAR_BIN_CREATE(lora_app_eui, 8,	8, VAR_BIN_PROTECT({0x70, 0xB3,	0xD5, 0x7E,	0xD0, 0x03,	0x31, 0xC9}));
VAR_BIN_CREATE(lora_dev_eui, 8,	8, VAR_BIN_PROTECT({0x00, 0x00,	0x00, 0x00,	0x00, 0x00,	0x0a, 0x01}));
VAR_BIN_CREATE(lora_app_key, 16, 16, VAR_BIN_PROTECT({0x6F,	0x7B, 0x80,	0xF7, 0xE4,	0xD0, 0xB9,	0xE5, 0x1F,	0xE9, 0xF8,	0x97, 0x64,	0x15, 0xBD,	0xD7}));
VAR_BIN_CREATE(lora_dev_addr, 4, 4,	VAR_BIN_PROTECT({0x01, 0x20, 0x02, 0x26}));
VAR_BIN_CREATE(lora_chan_mask, 10, 10, VAR_BIN_PROTECT({0x00,0x0f, 0x00,0x00, 0x00,0x00, 0x00,0x00,	0xff,0x00}));

uint16_t var_connretry	 = 30;

uint16_t var_polltimeout = 5;

enum setget_type {
	vtype_boolean =	0,
	vtype_uint8,
	vtype_uint16,
	vtype_uint32,
	vtype_str,
	vtype_binary,
	vtype_custom
};

enum setget_direction {
	vdir_read,
	vdir_write,
	vdir_readwrite
};

typedef	void* (*getter)(void);
typedef	void* (*setter)(void*);

enum var_save_id {
	id_none			=  0,
	id_devid		=  1,
	id_devtype		=  3,
	id_proto		=  6,
	id_autoconnect	=  7,
	id_lora_auth	= 16,
	id_appskey		= 17,
	id_nwkskey		= 18,
	id_appeui		= 19,
	id_deveui		= 20,
	id_appkey		= 21,
	id_devaddr		= 22,
	id_lora_class	= 23,
	id_lora_adr		= 24,
	id_datarate		= 25,
	id_chan_mask	= 26,
	id_rxdelay1		= 27,
	id_rxdelay2		= 28
};

struct key_setget_s	{
	const uint8_t *key;
	enum setget_type vtype;
	enum setget_direction vdir;
	void *variable;
	setter set;
	getter get;
	enum var_save_id save_id;
};

static struct key_setget_s setget[]	= {
	{"uart_echo", vtype_boolean, vdir_readwrite, &var_echo,	NULL, NULL,	id_none},
	{"devid", vtype_str, vdir_readwrite, &var_devid, NULL, NULL, id_devid},
	{"devid_gen", vtype_uint32,	vdir_read, &var_device_id, NULL, NULL, id_none},
	{"firmware", vtype_str,	vdir_read, &var_firmware, NULL,	NULL, id_none},
	{"mfirmware", vtype_str, vdir_read,	&var_mfirmware,	NULL, NULL,	id_none},
	{"battery",	vtype_uint8, vdir_read,	&var_battery, NULL,	NULL, id_none},
	{"connected", vtype_boolean, vdir_read,	&var_connected,	NULL, NULL,	id_none},
	{"connect",	vtype_boolean, vdir_readwrite, &var_connect, NULL, NULL, id_none},
	{"auto_connect", vtype_boolean,	vdir_readwrite,	&var_auto_connect, NULL, NULL, id_autoconnect},
	{"proto", vtype_str, vdir_readwrite, &var_proto, NULL, NULL, id_proto},
	{"enabled",	vtype_boolean, vdir_readwrite, &var_enabled, NULL, NULL, id_none},
	{"sensor", vtype_str, vdir_readwrite, &var_sensor, NULL, id_none},
	{"sensor_board", vtype_str,	vdir_readwrite,	&var_sensor_board, NULL, id_none},
	{"leds", vtype_boolean,	vdir_readwrite,	&var_leds, NULL, NULL, id_none},
	{"connretry", vtype_uint16,	vdir_readwrite,	&var_connretry,	NULL, NULL,	id_none},
	{"devtype",	vtype_str, vdir_readwrite, &var_devtype, NULL, NULL, id_devtype},
	{"reboot", vtype_boolean, vdir_write, NULL,	(setter)reboot,	NULL, id_none},
	{"save", vtype_boolean,	vdir_readwrite,	NULL, (setter)flash_save, (getter)flash_load, id_none},
	{"flashclear", vtype_boolean, vdir_readwrite, NULL,	(setter)flash_clear, NULL, id_none},

	// LoRa
	{"auth",	vtype_str,	vdir_readwrite,	&var_lora_auth,			NULL, NULL,	id_lora_auth},
	{"class",	vtype_str,	vdir_readwrite,	&var_lora_class,		NULL, NULL,	id_lora_class},
	{"deveui",	vtype_binary, vdir_readwrite, &var_lora_dev_eui,	NULL, NULL,	id_deveui},
	{"adrenabled", vtype_boolean, vdir_readwrite,  &var_lora_adr,	NULL, NULL,	id_lora_adr},
	{"chanmask", vtype_binary, vdir_readwrite, &var_lora_chan_mask,	NULL, NULL,	id_chan_mask},
	{"datarate", vtype_uint8, vdir_readwrite, &var_lora_datarate,	NULL, NULL,	id_datarate},
	{"rxdelay1", vtype_uint16, vdir_readwrite, &var_lora_rxdelay1,	NULL, NULL,	id_rxdelay1},
	{"rxdelay2", vtype_uint16, vdir_readwrite, &var_lora_rxdelay2,	NULL, NULL,	id_rxdelay2},
	// LoRa	ABP
	{"appskey",	vtype_binary, vdir_readwrite, &var_lora_app_skey,	NULL, NULL,	id_appskey},
	{"nwkskey",	vtype_binary,	vdir_readwrite,	&var_lora_nwk_skey,	NULL, NULL,	id_nwkskey},
	// LoRa	OTAA
	{"appkey",	vtype_binary, vdir_readwrite, &var_lora_app_key,	NULL, NULL,	id_appkey},
	{"appeui",	vtype_binary, vdir_readwrite, &var_lora_app_eui,	NULL, NULL,	id_appeui},
	{"devaddr",	vtype_binary, vdir_readwrite, &var_lora_dev_addr,	NULL, NULL,	id_devaddr},

	// Help	Menu (TODO:	Implement!)
	{"help", vtype_boolean,	vdir_read, NULL, (setter) NULL,	(getter) NULL, id_none},
	{"menu", vtype_boolean,	vdir_read, NULL, (setter) NULL,	(getter) NULL, id_none},
	{"list", vtype_boolean,	vdir_read, NULL, (setter) NULL,	(getter) NULL, id_none},
	{"?",	 vtype_boolean,	vdir_read, NULL, (setter) NULL,	(getter) NULL, id_none},

	{"binary", vtype_boolean, vdir_readwrite, &var_binary, NULL, NULL, id_none},

	{NULL, vtype_custom, vdir_readwrite, NULL, NULL, NULL, id_none}
};

static struct nvs_fs fs;

int	vars_init(void)
{
	int	rc;

	rc = vars_flash_init();
	if (rc < 0)
		return rc;

	flash_load();

	return 0;
}

static int vars_flash_init(void)
{
	int	rc;
	//int rc = 0, cnt =	0, cnt_his = 0;
	//char buf[16];
	//u8_t key[8], longarray[128];
	//u32_t	reboot_counter = 0U, reboot_counter_his;
	struct flash_pages_info	info;

	/* define the nvs file system by settings with:
	 *	sector_size	equal to the pagesize,
	 *	3 sectors
	 *	starting at	DT_FLASH_AREA_STORAGE_OFFSET
	 */

	fs.offset =	FLASH_AREA_OFFSET(storage);
	rc = flash_get_page_info_by_offs(device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL),
									 fs.offset,	&info);
	if (rc)	{
			LOG_ERR("Unable	to get page	info");
			return -1;
	}
	fs.sector_size = info.size;
	fs.sector_count	= 3U;

	rc = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	if (rc)	{
		LOG_ERR("Flash Init	failed");
		return -2;
	}

	return 0;
}

static int vars_flash_read(int id, void	*buffer, int buf_sz)
{
	int	rc;
	rc = nvs_read(&fs, id, buffer, buf_sz);
	if (rc > 0)	{ /* item was found, show it */
		LOG_INF("Id: %d, found.", id);
	} else	 {/* item was not found, add it	*/
		LOG_ERR("No	address	found at id	%d", id);
	}

	return rc;
}

static int vars_flash_write(int	id,	void *buffer, int buf_sz)
{
	int	rc;

	rc = nvs_write(&fs,	id,	buffer,	buf_sz);
	if (rc >= 0) {
		return 0;
	}
	rc = nvs_write(&fs,	id,	buffer,	buf_sz);
	if (rc)	{
		LOG_ERR("Flash write of	id %d failed", id);
		return -1;
	}

	return 0;
}

static void* flash_clear(void)
{
	nvs_clear(&fs);
	return "1";
}

static void* flash_load(void)
{
	int	rc = 0;
	int	hindex = 0;
	struct var_str_s *vstr;
	struct var_bin_s *vbin;

	while (setget[hindex].key) {
		if (setget[hindex].save_id != id_none) {
			switch (setget[hindex].vtype) {
				case vtype_boolean:
					rc = vars_flash_read(setget[hindex].save_id, (uint8_t *)setget[hindex].variable, sizeof(uint8_t));
					break;
				case vtype_uint8:
					rc = vars_flash_read(setget[hindex].save_id, (uint8_t *)setget[hindex].variable, sizeof(uint8_t));
					break;
				case vtype_uint16:
					rc = vars_flash_read(setget[hindex].save_id, (uint16_t *)setget[hindex].variable, sizeof(uint16_t));
					break;
				case vtype_uint32:
					rc = vars_flash_read(setget[hindex].save_id, (uint32_t *)setget[hindex].variable, sizeof(uint32_t));
					break;
				case vtype_str:
					vstr = (struct var_str_s*)setget[hindex].variable;
					rc = vars_flash_read(setget[hindex].save_id, vstr->data, vstr->size);
					vstr->length = strlen(vstr->data);
					break;
				case vtype_binary:
					vbin = (struct var_bin_s*)setget[hindex].variable;
					rc = vars_flash_read(setget[hindex].save_id, vbin->data, vbin->maxsize);
					if (rc >= 0)
						vbin->length = rc;
					break;
				default:
					LOG_ERR("ERROR reading of type is not supported");
					break;
			}
		}
		hindex++;
	}

	if (rc < 0)	{
		return "-1";
	}

	return "1";
}

static void* flash_save(void* data)
{
	int	rc = 0;
	int	hindex = 0;
	struct var_str_s *vstr;
	struct var_bin_s *vbin;

	while (setget[hindex].key && rc	>= 0) {
		if (setget[hindex].save_id != id_none) {
			LOG_INF("Saving	variable %s	at %d",	setget[hindex].key,	setget[hindex].save_id);
			switch (setget[hindex].vtype) {
				case vtype_boolean:
					rc = vars_flash_write(setget[hindex].save_id, (uint8_t *)setget[hindex].variable, sizeof(uint8_t));
					break;
				case vtype_uint8:
					rc = vars_flash_write(setget[hindex].save_id, (uint8_t *)setget[hindex].variable, sizeof(uint8_t));
					break;
				case vtype_uint16:
					rc = vars_flash_write(setget[hindex].save_id, (uint16_t	*)setget[hindex].variable, sizeof(uint16_t));
					break;
				case vtype_uint32:
					rc = vars_flash_write(setget[hindex].save_id, (uint32_t	*)setget[hindex].variable, sizeof(uint32_t));
					break;
				case vtype_str:
					vstr = (struct var_str_s*)setget[hindex].variable;
					rc = vars_flash_write(setget[hindex].save_id, vstr->data, vstr->length + 1);
					break;
				case vtype_binary:
					vbin = (struct var_bin_s*)setget[hindex].variable;
					rc = vars_flash_write(setget[hindex].save_id, vbin->data, vbin->length);
					break;
				default:
					LOG_ERR("ERROR writing of type is not supported");
					break;
			}
		}
		hindex++;
	}

	if (rc < 0)	{
		return "-1";
	}

	return "1";
}

static int setget_find_key(char	*key)
{
	int	hindex = 0;

	while (setget[hindex].key) {
		if (strcmp(setget[hindex].key, key)	== 0) {
			return hindex;
		}
		hindex++;
	}

	return -1;
}

// List	commmand for help (?) menu.
int	list_next_command(char *command)
{
	static int hindex =	0;

	if (setget[hindex].key)
	{
		// Copy	string and increment index for next	call.
		strcpy(command,	setget[hindex++].key);
		return hindex;
	}

	return hindex =	0;
}

static int hexdigit_to_value(uint8_t digit)
{
	if (digit >= '0' &&	digit <= '9') {
		return digit - '0';
	}
	if (digit >= 'a' &&	digit <= 'f') {
		return digit - 'a' + 10;
	}
	if (digit >= 'A' &&	digit <= 'F') {
		return digit - 'A' + 10;
	}
	return -1;
}

enum verr_codes	vars_set(char *key,	char *value, int vlen, char	**value_str)
{
	int	hindex = setget_find_key(key);
	static char	buffer[64];
	struct var_str_s *vstr;
	struct var_bin_s *vbin;
	void *variable = NULL;
	int	digval;

	if (hindex < 0)	{
		return verr_inv_key;
	}

	if (value == NULL || value[0] == 0)	{
		return verr_inv_value;
	}

	if (setget[hindex].vdir	== vdir_read) {
		return verr_inv_access;
	}

	variable = setget[hindex].variable;
	*value_str = NULL;

	if (variable) {
		switch (setget[hindex].vtype) {
			case vtype_boolean:
				*((bool	*)setget[hindex].variable) = atoi(value) ? true	: false;
				*value_str = *((bool *)setget[hindex].variable)	? "1" :	"0";
				break;
			case vtype_uint8:
				*((uint8_t *)setget[hindex].variable) =	atoi(value);
				sprintf(buffer,	"%d", *((uint8_t *)setget[hindex].variable));
				*value_str = buffer;
				break;
			case vtype_uint16:
				*((uint16_t	*)setget[hindex].variable) = atoi(value);
				sprintf(buffer,	"%d", *((uint16_t *)setget[hindex].variable));
				*value_str = buffer;
				break;
			case vtype_uint32:
				*((uint32_t	*)setget[hindex].variable) = atoi(value);
				sprintf(buffer,	"%d", *((uint32_t *)setget[hindex].variable));
				*value_str = buffer;
				break;
			case vtype_str:
				vstr = (struct var_str_s*)setget[hindex].variable;
				strncpy(vstr->data,	value, vstr->size);
				vstr->length = strlen(vstr->data);
				*value_str = vstr->data;
				break;
			case vtype_binary:
				vbin = (struct var_bin_s*)setget[hindex].variable;
				/* value should	be of the form 0xAABBCCDD... so	the	length should be 2 + 2 * bytes */
				vlen = strlen(value) - 2;
				if (vlen < vbin->minsize*2)
					return verr_inv_value;
				if (vlen > vbin->maxsize*2)
					return verr_inv_value;
				if (value[0] !=	'0'	|| value[1]	!= 'x')
					return verr_inv_value;
				vlen = vlen	/ 2;
				for	(int index = 0 ; index < vlen ;	index++) {
					/* Convert to hex value	*/
					digval = hexdigit_to_value(value[2 + index*2 + 0]);
					if (digval < 0)
						return verr_inv_value;
					buffer[index] =	digval * 16;
					digval = hexdigit_to_value(value[2 + index*2 + 1]);
					if (digval < 0)
						return verr_inv_value;
					buffer[index] += digval;
				}
				memcpy(vbin->data, buffer, vlen);
				vbin->length = vlen;
				//strcpy(buffer, value);
				*value_str = value;
				break;
			default:
				return verr_inv_type;
		}
	}

	if (setget[hindex].set)	{
		char * result =	setget[hindex].set(value);
		if (*value_str == NULL)	{
			if (result == NULL)	{
				*value_str = value;
			}
			else {
				*value_str = result;
			}
		}
	}

	return verr_success;
}

enum verr_codes	vars_get(char *key,	char *value, int vlen, char	**value_str)
{
	int	hindex = setget_find_key(key);
	static char	buffer[64];
	struct var_str_s *vstr;
	struct var_bin_s *vbin;
	void *variable = NULL;
	int	index;

	if (hindex < 0)	{
		return verr_inv_key;
	}

	if (value != NULL && value[0] != 0)	{
		return verr_inv_value;
	}

	if (setget[hindex].vdir	== vdir_write) {
		return verr_inv_access;
	}

	variable = setget[hindex].variable;

	*value_str = "ERROR";

	switch (setget[hindex].vtype) {
		case vtype_boolean:
			if (variable) {
				*value_str = *((bool *)variable) ? "1" : "0";
			}
			if (setget[hindex].get)	{
				*value_str = setget[hindex].get() ?	"1"	: "0";
			}
			break;
		case vtype_uint8:
			if (variable) {
				sprintf(buffer,	"%d", *((uint8_t *)variable));
				*value_str = buffer;
			}
			if (setget[hindex].get)	{
				sprintf(buffer,	"%d", (uint32_t)setget[hindex].get());
				*value_str = buffer;
			}
			break;
		case vtype_uint16:
			if (variable) {
				sprintf(buffer,	"%d", *((uint16_t *)variable));
				*value_str = buffer;
			}
			if (setget[hindex].get)	{
				sprintf(buffer,	"%d", (uint32_t)setget[hindex].get());
				*value_str = buffer;
			}
			break;
		case vtype_uint32:
			if (variable) {
				sprintf(buffer,	"%d", *((uint32_t *)variable));
				*value_str = buffer;
			}
			if (setget[hindex].get)	{
				sprintf(buffer,	"%d", (uint32_t)setget[hindex].get());
				*value_str = buffer;
			}
			break;
		case vtype_str:
			if (variable) {
				vstr = (struct var_str_s*)variable;
				*value_str = vstr->data;
			}
			if (setget[hindex].get)	{
				*value_str = (char*)setget[hindex].get();
			}
			break;
		case vtype_binary:
			if (variable) {
				vbin = (struct var_bin_s*)variable;
				buffer[0] =	'0';
				buffer[1] =	'x';
				for	(index = 0 ;index <	vbin->length ; index++)	{
					sprintf(&buffer[index*2+2],	"%02x",	vbin->data[index]);
				}
				*value_str = buffer;
			}
			if (setget[hindex].get)	{
				*value_str = (char*)setget[hindex].get();
			}
			break;
		default:
			return -3;
	}

	return 0;
}


void* reboot(void *unused)
{
	sys_reboot(0);
	return NULL;
}

int	save_vars_config(void)
{
	int	ret	= -1;

	ret	= atoi((char *)	flash_save(NULL));

	return ret;
}
