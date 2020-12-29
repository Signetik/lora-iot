/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
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

//#include "LTE_task.h"
#//include "gps_task.h"
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
void* gps_enable(void);
void* gps_configure(void);
void* gps_read(void);

static int vars_flash_init(void);
static int vars_flash_read(int id, void	*buffer, int buf_sz);
static void* flash_save(void* data);
static void* flash_load(void);

#define	VAR_STR_CREATE(_name_,_size_,_default_)	\
	uint8_t	_name_[_size_]	= _default_; \
	struct var_str_s var_##_name_ =	{ \
		_name_,	sizeof(_default_), sizeof(_name_) \
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
VAR_STR_CREATE(proto,		  8, "coap");
VAR_STR_CREATE(current_proto, 8, "none");
VAR_STR_CREATE(sensor, 32, "temp_humid");
VAR_STR_CREATE(sensor_board, 32, "THA");

VAR_STR_CREATE(host,  65, "iot.aws.signetik.com");
uint16_t var_port =	5715;
VAR_STR_CREATE(apikey,	65,	"");
VAR_STR_CREATE(uri,	 65, "*");


//VAR_STR_CREATE(subtopic,VAR_TOPIC_SIZE, "devices/sig-test-device-1/messages/devicebound/#");
VAR_STR_CREATE(subtopic1, VAR_TOPIC_SIZE, "devices/sig-test-device-2/messages/devicebound/#");
VAR_STR_CREATE(subtopic2, VAR_TOPIC_SIZE, "$iothub/methods/POST/#");
VAR_STR_CREATE(subtopic3, VAR_TOPIC_SIZE, "$iothub/twin/res/#");
VAR_STR_CREATE(subtopic4, VAR_TOPIC_SIZE, "");
VAR_STR_CREATE(subtopic5, VAR_TOPIC_SIZE, "");

//VAR_STR_CREATE(pubtopic,VAR_TOPIC_SIZE, "devices/sig-test-device-1/messages/events/");
VAR_STR_CREATE(pubtopic,  VAR_TOPIC_SIZE, "devices/sig-test-device-2/messages/events/");

uint8_t	 var_qos		= 1;
//uint16_t var_keepalive	= CONFIG_MQTT_KEEPALIVE;
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
bool var_nbiot		= false;
bool var_binary		= false;
bool var_leds		= true;

// Report Names
uint8_t	report[VAR_MAX_REPORTS][VAR_REPORT_NAME_SIZE] =	{
	"queue1",
	"queue2",
	"queue3",
	"queue4",
	"queue5",
};

// Reports
struct var_str_s var_report[VAR_MAX_REPORTS] = {
	{ report[0], 7,	VAR_REPORT_NAME_SIZE },
	{ report[1], 7,	VAR_REPORT_NAME_SIZE },
	{ report[2], 7,	VAR_REPORT_NAME_SIZE },
	{ report[3], 7,	VAR_REPORT_NAME_SIZE },
	{ report[4], 7,	VAR_REPORT_NAME_SIZE },
};

// LoRa	Vars
uint8_t	 var_lora_mode = 0;
VAR_STR_CREATE(lora_app_skey, 8, "");
VAR_STR_CREATE(lora_nwk_skey, 8, "");
VAR_STR_CREATE(lora_app_eui, 16, "");
VAR_STR_CREATE(lora_dev_eui, 8,	"");
VAR_STR_CREATE(lora_join_eui, 8, "");
VAR_STR_CREATE(lora_app_key, 16, "");

// GPS Vars
uint16_t var_gpsinterval =	0;
bool  var_gpsenabled  =	false;
bool  var_gps		  =	false;
bool  var_gpspos	  =	false;
bool  var_gpsdate	  =	false;
bool  var_gpstime	  =	false;
bool  var_gpsdatetime =	false;
bool  var_gpsnema	  =	false;
bool  var_gpsgga	  =	false;
bool  var_gpsgll	  =	false;
bool  var_gpsrmc	  =	false;
bool  var_gpsvtg	  =	false;
bool  var_gpszda	  =	false;

// GPS raw NMEA
// static char	 nmea_strings[10][NRF_GNSS_NMEA_MAX_LEN];
// static u32_t	 nmea_string_cnt;

// GPS Names
uint8_t	gps_report[VAR_MAX_GPS_REPORTS][VAR_GPS_NAME_SIZE]	= {
	"gps1",
	"gps2",
	"gps3",
	"gps4",
	"gps5",
};

// GPS Reports
struct var_str_s var_gpsreport[VAR_MAX_GPS_REPORTS]	= {
	{ gps_report[0], 5,	VAR_GPS_NAME_SIZE },
	{ gps_report[1], 5,	VAR_GPS_NAME_SIZE },
	{ gps_report[2], 5,	VAR_GPS_NAME_SIZE },
	{ gps_report[3], 5,	VAR_GPS_NAME_SIZE },
	{ gps_report[4], 5,	VAR_GPS_NAME_SIZE },
};

// NMEA	Names
uint8_t	gps_nmea[VAR_MAX_NMEA_REPORTS][VAR_NMEA_NAME_SIZE]	= {
	"gga", // Global Positioning System	Fix	Data
	"gll", // Geographic Position, Latitude	/ Longitude	and	time
	"rmc", // Recommended minimum specific GPS/Transit data
	"vtg", // Track	Made Good and Ground Speed (Velocity)
	"zda", // Date & Time
};

// NMEA
struct var_str_s var_gpsnmea[VAR_MAX_NMEA_REPORTS] = {
	{ gps_nmea[0], 4, VAR_NMEA_NAME_SIZE },
	{ gps_nmea[1], 4, VAR_NMEA_NAME_SIZE },
	{ gps_nmea[2], 4, VAR_NMEA_NAME_SIZE },
	{ gps_nmea[3], 4, VAR_NMEA_NAME_SIZE },
	{ gps_nmea[4], 4, VAR_NMEA_NAME_SIZE },
};

uint16_t var_connretry	 = 30;
uint16_t var_sectag		 = 0;
uint16_t var_sectagquery = 21;

VAR_STR_CREATE(fotahost,	 32, "fota.signetik.com");
VAR_STR_CREATE(fotahostname, 32, "fota.signetik.com");
VAR_STR_CREATE(fotafile,	 64, "sigcell/v1.0.4.bin");
uint16_t var_fotastate	  =	0;
uint16_t var_fotasubstate =	0;
uint16_t var_fotasectag	  =	0;

uint16_t var_dlcount	   = 0;
uint16_t var_dlchunks	   = 0;
uint32_t var_dloffset	   = 0;
uint16_t var_dlretries	   = 0;	//CONFIG_FOTA_SOCKET_RETRIES;
uint16_t var_dlretrycount =	0; //CONFIG_FOTA_SOCKET_RETRIES;

bool var_dfuready =	false;
bool var_fwupdate =	false;

uint16_t var_polltimeout = 5;

enum setget_type {
	vtype_boolean =	0,
	vtype_uint8,
	vtype_uint16,
	vtype_uint32,
	vtype_str,
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
	id_nbiot		=  2,
	id_devtype		=  3,
	id_fotahost		=  4,
	id_fotahostname	=  5,
	id_proto		=  6,
	id_autoconnect	=  7,
	id_queue1		= 11,
	id_queue2		= 12,
	id_queue3		= 13,
	id_queue4		= 14,
	id_queue5		= 15,
	id_loramode,
	id_appskey,
	id_nwkskey,
	id_appeui,
	id_deveui,
	id_joineui,
	id_appkey
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
	{"user", vtype_str,	vdir_readwrite,	&var_user, NULL, NULL, id_none},
	{"pw", vtype_str, vdir_readwrite, &var_pw, NULL, NULL, id_none},
	{"server_address", vtype_str, vdir_readwrite, &var_host, NULL, NULL, id_none},
	{"api_key",	vtype_str, vdir_readwrite, &var_apikey,	NULL, NULL,	id_none},
	{"uri",	vtype_str, vdir_readwrite, &var_uri, NULL, NULL, id_none},
	{"server_port",	vtype_uint16, vdir_readwrite, &var_port, NULL, NULL, id_none},
	{"subtopic", vtype_str,	vdir_readwrite,	&var_subtopic1,	NULL, NULL,	id_none},
	{"subtopic1", vtype_str, vdir_readwrite, &var_subtopic1, NULL, NULL, id_none},
	{"subtopic2", vtype_str, vdir_readwrite, &var_subtopic2, NULL, NULL, id_none},
	{"subtopic3", vtype_str, vdir_readwrite, &var_subtopic3, NULL, NULL, id_none},
	{"subtopic4", vtype_str, vdir_readwrite, &var_subtopic4, NULL, NULL, id_none},
	{"subtopic5", vtype_str, vdir_readwrite, &var_subtopic5, NULL, NULL, id_none},
	{"pubtopic", vtype_str,	vdir_readwrite,	&var_pubtopic, NULL, NULL, id_none},
	{"qos",	vtype_uint8, vdir_readwrite, &var_qos, NULL, NULL, id_none},
	{"keepalive", vtype_uint16,	vdir_readwrite,	&var_keepalive,	NULL, NULL,	id_none},
	{"devid_gen", vtype_uint32,	vdir_read, &var_device_id, NULL, NULL, id_none},
	//{"connection_retry_interval",	vtype_uint16, vdir_readwrite, &var_retry_interval, NULL, NULL},
	{"firmware", vtype_str,	vdir_read, &var_firmware, NULL,	NULL, id_none},
	{"mfirmware", vtype_str, vdir_read,	&var_mfirmware,	NULL, NULL,	id_none},
	{"battery",	vtype_uint8, vdir_read,	&var_battery, NULL,	NULL, id_none},
	{"connected", vtype_boolean, vdir_read,	&var_connected,	NULL, NULL,	id_none},
	{"connect",	vtype_boolean, vdir_readwrite, &var_connect, NULL, NULL, id_none},
	{"auto_connect", vtype_boolean,	vdir_readwrite,	&var_auto_connect, NULL, NULL, id_autoconnect},
	//{"queue_mark", vtype_uint32, vdir_read, &var_queue_mark, NULL, NULL},
	//{"push_mark",	vtype_uint32, vdir_read, &var_push_mark, NULL, NULL},
//	{"cell_rsrp", vtype_uint32,	vdir_read, NULL, NULL, (getter)LTE_get_signal_strength,	id_none},
//	{"cell_mode", vtype_str, vdir_read,	NULL, NULL,	(getter)LTE_get_mode, id_none},
	{"proto", vtype_str, vdir_readwrite, &var_proto, NULL, NULL, id_proto},
	{"enabled",	vtype_boolean, vdir_readwrite, &var_enabled, NULL, NULL, id_none},
	{"sensor_poll",	vtype_boolean, vdir_readwrite, &var_sensor_poll, (setter)sensor_enable,	NULL, id_none},
	{"sensor", vtype_str, vdir_readwrite, &var_sensor, NULL, id_none},
	{"sensor_board", vtype_str,	vdir_readwrite,	&var_sensor_board, NULL, id_none},
	{"imei", vtype_str,	vdir_read, &var_imei, NULL,	NULL, id_none},
	{"imsi", vtype_str,	vdir_read, NULL, NULL, (getter)imsi_read, id_none},
	{"iccid", vtype_str, vdir_read,	NULL, NULL,	(getter)iccid_read,	id_none},
	{"nbiot", vtype_boolean, vdir_readwrite, &var_nbiot, NULL, NULL, id_nbiot},
//	{"time", vtype_str,	vdir_read, NULL, NULL, (getter)LTE_get_time, id_none},
	{"leds", vtype_boolean,	vdir_readwrite,	&var_leds, NULL, NULL, id_none},
	{"queue1", vtype_str, vdir_readwrite, &var_report[0], (setter)sigconfig_set_report_names, NULL,	id_queue1},
	{"queue2", vtype_str, vdir_readwrite, &var_report[1], (setter)sigconfig_set_report_names, NULL,	id_queue2},
	{"queue3", vtype_str, vdir_readwrite, &var_report[2], (setter)sigconfig_set_report_names, NULL,	id_queue3},
	{"queue4", vtype_str, vdir_readwrite, &var_report[3], (setter)sigconfig_set_report_names, NULL,	id_queue4},
	{"queue5", vtype_str, vdir_readwrite, &var_report[4], (setter)sigconfig_set_report_names, NULL,	id_queue5},
	{"connretry", vtype_uint16,	vdir_readwrite,	&var_connretry,	NULL, NULL,	id_none},
	{"cacert", vtype_str, vdir_readwrite, NULL,	(setter)cacert_write, (getter)cacert_read, id_none},
	{"privcert", vtype_str,	vdir_readwrite,	NULL, (setter)privcert_write, (getter)privcert_read, id_none},
	{"privkey",	vtype_str, vdir_readwrite, NULL, (setter)privkey_write,	(getter)privkey_read, id_none},
	{"sectag", vtype_uint16, vdir_readwrite, &var_sectag, NULL,	NULL, id_none},
	{"sectagq",	vtype_uint16, vdir_readwrite, &var_sectagquery,	NULL, NULL,	id_none},
	{"fotahost", vtype_str,	vdir_readwrite,	&var_fotahost, NULL, NULL, id_fotahost},
	{"fotahostname", vtype_str,	vdir_readwrite,	&var_fotahostname, NULL, NULL, id_fotahostname},
	{"fotafile", vtype_str,	vdir_readwrite,	&var_fotafile, NULL, NULL, id_none},
	{"fotasubfile",	vtype_str, vdir_readwrite, &var_fotafile, NULL,	NULL, id_none},
	{"fotastart", vtype_boolean, vdir_write, NULL, (setter)fota_start, NULL, id_none},
	{"fotasubstart", vtype_boolean,	vdir_write,	NULL, (setter)fotasub_start, NULL, id_none},
	{"fotastate", vtype_uint16,	vdir_read, &var_fotastate, NULL, NULL, id_none},
	{"fotasubstate", vtype_uint16, vdir_read, &var_fotasubstate, NULL, NULL, id_none},
	{"fotasectag", vtype_uint16, vdir_readwrite, &var_fotasectag, NULL,	NULL, id_none},
	{"fotasuback", vtype_boolean, vdir_write, NULL,	(setter)fotasuback_set,	NULL, id_none},
	{"dlcount",	vtype_uint16, vdir_read, &var_dlcount, NULL, NULL, id_none},
	{"dlchunks", vtype_uint16, vdir_readwrite, &var_dlchunks, NULL,	NULL, id_none},
	{"dloffset", vtype_uint32, vdir_readwrite, &var_dloffset, NULL,	NULL, id_none},
	{"dlretries", vtype_uint16,	vdir_read, &var_dlretries, NULL, NULL, id_none},
	{"dlretrycount", vtype_uint16, vdir_read, &var_dlretrycount, NULL, NULL, id_none},
	{"fwupdate", vtype_boolean,	vdir_readwrite,	&var_fwupdate, NULL, NULL, id_none},
	{"devtype",	vtype_str, vdir_readwrite, &var_devtype, NULL, NULL, id_devtype},
	{"polltimeout",	vtype_uint16, vdir_readwrite, &var_polltimeout,	NULL, NULL,	id_none},
	{"at", vtype_str, vdir_write, NULL,	(setter)at_set,	NULL, id_none},
	{"reboot", vtype_boolean, vdir_write, NULL,	(setter)reboot,	NULL, id_none},
	{"save", vtype_boolean,	vdir_readwrite,	NULL, (setter)flash_save, (getter)flash_load, id_none},

	{"loramode",	vtype_uint8,  vdir_readwrite, &var_lora_mode, NULL,	NULL, id_loramode},
	// LoRa	ABP
	{"app_skey",	vtype_str, vdir_readwrite, &var_lora_app_skey,	NULL, NULL,	id_appskey},
	{"nwk_skey", vtype_str,	vdir_readwrite,	&var_lora_nwk_skey,	NULL, NULL,	id_nwkskey},
	{"app_eui",	vtype_str, vdir_readwrite, &var_lora_app_eui,	NULL, NULL,	id_appeui},

	// LoRa	OTAA
	{"dev_eui",	vtype_str, vdir_readwrite, &var_lora_dev_eui,	NULL, NULL,	id_deveui},
	{"join_eui", vtype_str,	vdir_readwrite,	&var_lora_join_eui,	NULL, NULL,	id_joineui},
	{"app_key",	vtype_str, vdir_readwrite, &var_lora_app_key,	NULL, NULL,	id_appkey},

	// GPS API (TODO: C.Lawson -- Finish!)
	{"gpsenable",	vtype_boolean, vdir_write,	   &var_gpsenabled,	 (setter)gps_enable, NULL, id_none},
	{"gpsenabled",	vtype_boolean, vdir_read,	   &var_gpsenabled,	 NULL, NULL, id_none},
	{"gpsinterval",	vtype_uint16,  vdir_readwrite, &var_gpsinterval, NULL, NULL, id_none},
	{"gpsreport",	vtype_str,	   vdir_readwrite, &var_gpsreport,	 NULL, NULL, id_none}, // Support string of	report fields (pos,	date, time,	etc.)
	{"gps",			vtype_boolean, vdir_read,	   &var_gps,		 NULL, (getter)gps_read, id_none},
	{"gpspos",		vtype_boolean, vdir_read,	   &var_gpspos,		 NULL, NULL, id_none},
	{"gpsdate",		vtype_boolean, vdir_read,	   &var_gpsdate,	 NULL, NULL, id_none},
	{"gpstime",		vtype_boolean, vdir_read,	   &var_gpstime,	 NULL, NULL, id_none},
	{"gpsdatetime",	vtype_boolean, vdir_read,	   &var_gpsdatetime, NULL, NULL, id_none},
	{"nmea",		vtype_boolean, vdir_read,	   &var_gpsnmea,	 NULL, NULL, id_none}, // Support string of	GGA, RMC, VTG, etc.
	{"gga",			vtype_boolean, vdir_read,	   &var_gpsgga,		 NULL, NULL, id_none},
	{"gll",			vtype_boolean, vdir_read,	   &var_gpsgll,		 NULL, NULL, id_none},
	{"rmc",			vtype_boolean, vdir_read,	   &var_gpsrmc,		 NULL, NULL, id_none},
	{"vtg",			vtype_boolean, vdir_read,	   &var_gpsvtg,		 NULL, NULL, id_none},
	{"zda",			vtype_boolean, vdir_read,	   &var_gpszda,		 NULL, NULL, id_none},

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

static void* flash_load(void)
{
	int	rc = 0;
	int	hindex = 0;
	struct var_str_s *vstr;

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

enum verr_codes	vars_set(char *key,	char *value, int vlen, char	**value_str)
{
	int	hindex = setget_find_key(key);
	static char	buffer[32];
	struct var_str_s *vstr;
	void *variable = NULL;

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
			default:
				return verr_inv_type;
		}
	}

	if (setget[hindex].set)	{
		char * result =	setget[hindex].set(value);
		if (*value_str == NULL)	{
			*value_str = result;
		}
	}

	return verr_success;
}

enum verr_codes	vars_get(char *key,	char *value, int vlen, char	**value_str)
{
	int	hindex = setget_find_key(key);
	static char	buffer[32];
	struct var_str_s *vstr;
	void *variable = NULL;

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
		default:
			return -3;
	}

	return 0;
}

void* cacert_write(void* cert)
{
	char* result;

	result = LTE_set_certificate(var_sectagquery, 0, cert);

	return result;
}

void* privcert_write(void* cert)
{
	char* result;

	result = LTE_set_certificate(var_sectagquery, 1, cert);

	return result;
}

void* privkey_write(void* cert)
{
	char* result;

	result = LTE_set_certificate(var_sectagquery, 2, cert);

	return result;
}

void* cacert_read(void)
{
	char* result;

	result = LTE_get_certificate(var_sectagquery, 0);

	return result;
}

void* privcert_read(void)
{
	char* result;

	result = LTE_get_certificate(var_sectagquery, 1);

	return result;
}

void* privkey_read(void)
{
	char* result;

	result = LTE_get_certificate(var_sectagquery, 2);

	return result;
}

void* fota_start(void *var)
{
	if (var_fotastate == 0)	{
		var_fotastate =	1;
		return "1";
	}
	return "0";
}

void* fotasub_start(void *var) {
	if (var_fotasubstate ==	0) {
		var_fotasubstate = 1;
		return "1";
	}
	return "0";
}

void* imsi_read(void)
{
	char *result;

	result = LTE_get_imsi();

	return result;
}

void* iccid_read(void)
{
	char *result;
	int	index;

	result = LTE_get_iccid();

	if (strlen(result) > 2)	{
		for	(index = 0 ; index < strlen(result)	; index	+= 2) {
			result[index] ^= result[index+1];
			result[index+1]	^= result[index];
			result[index] ^= result[index+1];
		}
	}

	return result;
}

void* fotasuback_set(void* arg)
{
	if (lte_fotasuback() ==	0) {
		return "1";
	}

	return "0";
}

void* at_set(void* arg)
{
	char *p	= (char*)arg;
	if (p[0] ==	'"') {
		p++;
		if (p[strlen(p)-1] == '"') {
			p[strlen(p)-1] = 0;
		}
	}
	return LTE_at(p);
}

void* reboot(void *unused)
{
	sys_reboot(0);
	return NULL;
}

/****************************************
 *			 Sensor	API	Commands		   *
 ****************************************/

///	Enable / Disable sensor	polling.
void* sensor_enable(void)
{
	LOG_DBG("Var: Sensor polling Enabled = %d\r\n",	var_sensor_poll);

	char *result = sensor_enable_polling(var_sensor_poll);

	return result;
}

/****************************************
 *			 GPS API Commands			*
 ****************************************/

///	Enable / Disable GPS.
void* gps_enable(void)
{
	LOG_DBG("Var: GPS Enabled =	%d\r\n", var_gpsenabled);

	char *result = GPS_enable_gps(var_gpsenabled);

	return result;
}

///	Configure GPS.
void* gps_configure(void)
{
	char *result = GPS_configure_gps();

	return result;
}

///	Get	GPS	data.
void* gps_read(void)
{
	char *result = GPS_get_gps();

	return result;
}
