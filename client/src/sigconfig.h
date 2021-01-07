/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#ifndef	SIGCONFIG_H_
#define	SIGCONFIG_H_

#include <stdio.h>
#include <string.h>
#include <zephyr.h>

#include "vars.h"

enum cbor_data_type
{
	CBOR_ERROR = 0,
	CBOR_INT8,
	CBOR_INT16,
	CBOR_INT32,
	CBOR_RAW,
	CBOR_TEXT,
	CBOR_FLOAT,
	CBOR_FLOAT_REVERSE,
	CBOR_DOUBLE,
	CBOR_BOOL,
};

enum sigcfg_protocol
{
	PROTO_ERROR	= 0,
	MODBUS,
	I2C,
	SPI,
	MEMORY,
};

struct report_data_storage_s {
	uint32_t ts_s;
	uint32_t ts_ms;
	uint32_t sequence;
};

struct field_data_storage_s	{
	uint32_t *data;
	uint8_t	size;
};

struct action_data_storage_s {
	uint32_t *data;
	uint8_t	size;
};

struct modbus_action_s
{
	uint8_t	slave_addr;
	uint8_t	opcode;
	uint16_t reg_addr;
	uint16_t num;
	struct action_data_storage_s storage;
};

struct i2c_action_s
{
	uint8_t	slave_addr;
	char* bus_name;
	uint8_t	reg_addr;
	uint8_t	wlen;
	uint8_t	rlen;
	struct action_data_storage_s storage;
};

struct memory_action_s
{
	uint16_t reg_addr;
	struct action_data_storage_s storage;
};

struct field_s
{
	enum sigcfg_protocol protocol;
	enum cbor_data_type	type;
	uint8_t	field_name [VAR_FIELD_NAME_SIZE];
	uint8_t	num_actions;
	union action_u {
		struct modbus_action_s modbus;
		struct i2c_action_s	i2c;
		struct memory_action_s memory;
	} action[5];
	struct field_data_storage_s	storage[VAR_REPORT_QUEUE_SIZE];
};

struct report_s
{
	uint8_t	report_name	[VAR_REPORT_NAME_SIZE];
	uint32_t sample_interval_s;
	uint32_t next_sample_time;
	uint32_t report_interval_s;
	uint8_t	num_fields[VAR_REPORT_QUEUE_SIZE];
	uint16_t max_records;
	uint8_t	queue_index;
	uint8_t	queue_count;

	struct report_data_storage_s storage[VAR_REPORT_QUEUE_SIZE];
	struct field_s fields[VAR_MAX_FIELDS];
};

struct sigconfig_master_s
{
	uint8_t	product_type[33];
	uint8_t	num_reports;
	uint32_t sequence;
	uint8_t	field_count;
	uint8_t	field_index;
	struct report_s	reports[VAR_MAX_REPORTS];
};

extern struct sigconfig_master_s sigcfg_master;
extern struct k_sem	sem_report_queue;

static uint32_t	field_data[MAX_FIELD_DATA][MAX_FIELD_VAL_LEN];

void sigconfig_init(void);
bool sigconfig_execute(int64_t current_time, int64_t real_time);
void sigconfig_perform_actions(int index, struct report_s *	p_group);
void sigconfig_process_cbor_payload(uint8_t	*payload, uint32_t length);
bool sigconfig_ready_to_send(void);
bool sigconfig_get_records_to_send(int count, int idx, int *record_first, int *record_last);
bool sigconfig_full(void);
bool sigconfig_field_data_full(void);
uint32_t sigconfig_push(char *report, struct var_param_s *params, int param_count, int64_t real_time);

uint32_t get_next_action_time(void);

#endif /* SIGCONFIG_H_ */
