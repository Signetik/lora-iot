/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#ifndef	__VARS_H
#define	__VARS_H

#if	!defined(LINUX)
#include <zephyr.h>
#else
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#endif

#define	MAX_KEY_LEN	24
#define	MAX_VAL_LEN	128
#define	MAX_FIELD_VAL_LEN 16
#define	MAX_FIELD_DATA	25

#define	VAR_TOPIC_SIZE 128
#define	VAR_REPORT_NAME_SIZE 16
#define	VAR_FIELD_NAME_SIZE	MAX_KEY_LEN
#define	VAR_REPORT_QUEUE_SIZE 5

#define	VAR_MAX_REPORTS	5
#define	VAR_MAX_FIELDS	5

// GPS
#define	VAR_GPS_NAME_SIZE  16
#define	VAR_MAX_GPS_REPORTS	5
#define	VAR_MAX_GPS_FIELDS	5

// NMEA
#define	VAR_NMEA_NAME_SIZE	 4 // GGA, RMC,	VTG, etc.
#define	VAR_MAX_NMEA_REPORTS 5
#define	VAR_MAX_NMEA_FIELDS	 5

// LoRA
 /*	OTAA */
#define	JOIN_EUI_MAX_SZ	8
#define	DEV_EUI_MAX_SZ	8
#define	APP_KEY_MAX_SZ 16

/* ABP */
#define	APP_SKEY_MAX_SZ	16
#define	NWK_SKEY_MAX_SZ	16
#define	APP_EUI_MAX_SZ	 8


struct var_param_s {
	uint8_t	key[MAX_KEY_LEN];
	uint8_t	*value;
	int	vlen;
};

struct var_str_s {
	uint8_t	*data;
	int	length;
	int	size;
};

struct var_bin_s {
	uint8_t	*data;
	int	length;
	int	minsize;
	int	maxsize;
};

enum verr_codes	{
	verr_success = 0,
	verr_inv_key = -1,
	verr_inv_value = -2,
	verr_inv_type =	-3,
	verr_inv_access	= -4,
};

extern bool	var_echo;

extern struct var_str_s	var_devid;
extern struct var_str_s	var_user;
extern struct var_str_s	var_pw;
extern struct var_str_s	var_host;
extern struct var_str_s	var_apikey;
extern struct var_str_s	var_uri;

extern uint16_t	var_port;

extern struct var_str_s	var_subtopic1;
extern struct var_str_s	var_subtopic2;
extern struct var_str_s	var_subtopic3;
extern struct var_str_s	var_subtopic4;
extern struct var_str_s	var_subtopic5;
extern struct var_str_s	var_pubtopic;
extern uint8_t var_qos;
extern uint16_t	var_keepalive;

extern uint32_t	var_device_id;
extern struct var_str_s	var_firmware;
extern struct var_str_s	var_mfirmware;
extern uint8_t var_battery;
extern bool	var_connected;
extern bool	var_connect;
extern bool	var_auto_connect;
extern uint32_t	var_queue_mark;
extern uint32_t	var_push_mark;
extern uint32_t	var_rsrp;

extern struct var_str_s	var_proto;
extern struct var_str_s	var_current_proto;
extern struct var_str_s	var_sensor;
extern struct var_str_s	var_sensor_board;

extern bool	var_enabled;
extern bool	var_tls;
extern bool	var_sensor_poll;

extern struct var_str_s	var_imei;
extern bool	var_nbiot;

extern struct var_str_s	var_report[VAR_MAX_REPORTS];

extern uint16_t	var_connretry;
extern uint16_t	var_sectag;

extern struct var_str_s	var_fotahost;
extern struct var_str_s	var_fotahostname;
extern struct var_str_s	var_fotafile;
extern uint16_t	var_fotastate;
extern uint16_t	var_fotasubstate;
extern uint16_t	var_fotasectag;

extern uint16_t	var_dlcount;
extern uint16_t	var_dlchunks;
extern uint32_t	var_dloffset;
extern uint16_t	var_dlretries;
extern uint16_t	var_dlretrycount;

extern bool	var_dfuready;
extern bool	var_fwupdate;

extern struct var_str_s	var_devtype;
extern bool	var_binary;
extern bool	var_leds;
extern uint16_t	var_polltimeout;

extern bool	var_lora_adr;
extern uint8_t	var_lora_datarate;
extern struct var_str_s	var_lora_class;
extern struct var_str_s	var_lora_auth;
extern struct var_bin_s	var_lora_app_skey;
extern struct var_bin_s	var_lora_nwk_skey;
extern struct var_bin_s	var_lora_app_eui;
extern struct var_bin_s	var_lora_dev_eui;
extern struct var_bin_s	var_lora_app_key;
extern struct var_bin_s	var_lora_dev_addr;
extern struct var_bin_s	var_lora_chan_mask;

int	list_next_command(char *command);

enum verr_codes	vars_set(char *key,	char *value, int vlen, char	**value_str);
enum verr_codes	vars_get(char *key,	char *value, int vlen, char	**value_str);

int	vars_init(void);
int	save_vars_config(void);

#endif /* __VARS_H */
