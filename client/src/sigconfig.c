/*
 * sigconfig.c
 *
 * Created:	4/16/19	11:33:55 AM
 *	Author:	scottschmitt
 */
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <logging/log.h>
#include <sys/printk.h>
#include <drivers/uart.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>

#include <stdlib.h>
#include "sigconfig.h"
//#include "LTE_task.h"
//#include "main_task.h"
#include "signetik.h"
//#include "temp_humidity_task.h"
//#include "sensors/signetik_htu21d.h"

#include "cbor.h"
#include "coap.h"
//#include "modbus.h"

LOG_MODULE_REGISTER(sigconfig, CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

//u8_t make_i2c_transaction(int	index, struct i2c_action_s * action);

extern const k_tid_t sig_conf_id;

static bool	sigconfig_data_ready;
static int64_t sigconfig_current_time	= 0, sigconfig_real_time = 0;
static int group_number	= -1;
K_SEM_DEFINE(sem_sigconfig,	0, 1);
K_SEM_DEFINE(sem_report_queue, 1, 1);

void sigconfig_start(int64_t current_time, int64_t real_time)
{
	sigconfig_current_time = current_time;
	sigconfig_real_time	= real_time;
	sigconfig_data_ready = false;
	k_sem_give(&sem_sigconfig);
}

bool sigconfig_ready(void)
{
	return sigconfig_data_ready;
}

int	sigconfig_get_data(void)
{
	return group_number;
}

struct sigconfig_master_s sigcfg_master;
#if	defined(SUPPORT_SSR)
struct sigconfig_master_s sigcfg_master2;
#endif

uint32_t get_next_action_time(void);

uint32_t sigcfg_task_high_wm;
volatile bool need_settings	= true;
uint8_t	sigcfg_packet_space	[64];
int	sig_conf_record_num	=  0;
int	sig_conf_num_reports =	0;

static uint32_t	memory[256]	= {	0 };

extern int uart_send(uint8_t *buffer, int length);

uint32_t get_next_action_time(void)
{
	uint32_t next_time = 0xFFFFFFFF;

	for(uint8_t	i =	0; i < sigcfg_master.num_reports; i++)
	{
		if((sigcfg_master.reports[i].next_sample_time) < next_time )
			next_time =	sigcfg_master.reports[i].next_sample_time;
	}

	return next_time;
}

void perform_actions(int index,	struct report_s	* p_group)
{
	for(uint8_t	j =	0; j < p_group->num_fields;	j++)
	{

		if(p_group->fields[j].protocol == MODBUS)
		{
#if	defined(CONFIG_SENSOR_ADDON_MODBUS)
			//configure_modbus_uart();
			for(uint8_t	i =	0; i < p_group->fields[j].num_actions; i++)
			{
				LOG_DBG("sigconf_Modbus	transaction	%d", i);
				make_modbus_transaction(index, &p_group->fields[j].action[i].modbus);
				// p_group->fields[0].data = (parse	actions?? )
			}
			// usart_reset(&usart_modbus_instance);	// reset the uart so we	can	configure it next time,	especially if settings change
#else
			LOG_ERR("MODBUS	not	supported!");
#endif
		}
		if(p_group->fields[j].protocol == I2C)
		{
#if	defined(CONFIG_SENSOR_ADDON_LIGHT) || defined(CONFIG_SENSOR_ADDON_TEMPHUMID)
			// configure_modbus_uart();

			if (sensor_api)	{
				int	res	= -1;
				res	= sensor_api->fetch_sample(&sensor_state, p_group, j);
				if (res	!= 0)  {
					LOG_INF("Failed	to fetch sample.");
					k_sleep(1000);
				}
			}
			else {
				LOG_INF("Sensor	not	initialized.");
				k_sleep(1000);
			}

			// usart_reset(&usart_modbus_instance);	// reset the uart so we	can	configure it next time,	especially if settings change
#else
			LOG_ERR("MODBUS	not	supported!");
#endif
		}
		if(p_group->fields[j].protocol == MEMORY)
		{
			// TODO: Don't assume read
			for(uint8_t	i =	0; i < p_group->fields[j].num_actions; i++)
			{
				uint16_t addr =	p_group->fields[j].action[i].memory.reg_addr;

				if (addr < sizeof(memory) /	sizeof(memory[0])) {
					p_group->fields[j].action[i].memory.storage.data[index]	=
					memory[p_group->fields[j].action[i].memory.reg_addr];
				}
				else if	(addr >= 0x400 && addr < 0x800)	{
					p_group->fields[j].action[i].memory.storage.data[index]	=
					addr - 0x400;
				}
				else {
					p_group->fields[j].action[i].memory.storage.data[index]	=
					-1;
				}
			}

		}
	}
}

#define	TESTING
#if	defined(TESTING)
#define	SENSOR_RATE	60
#define	REPORT_RATE	120
#else
#define	SENSOR_RATE	60
#define	REPORT_RATE	300
#endif

// IoT Sensor Configurations
#ifdef CONFIG_SENSOR_ADDON_MODBUS
// ABO
#define	REPORT_GROUPS 32
#endif
#ifdef CONFIG_SENSOR_ADDON_TEMPHUMID
#define	REPORT_GROUPS 1
#endif
#ifdef CONFIG_SENSOR_ADDON_LIGHT
#define	REPORT_GROUPS 1
#endif

#define	MAX_FIELDS 5
#define	MAX_RECORDS	(REPORT_RATE/SENSOR_RATE * 3)

void sigconfig_init(void)
{
// TODO: Move this out of here
#ifdef CONFIG_SENSOR_ADDON_MODBUS
#define	DEVICE_ADDRESS 1

	configure_modbus_uart();

	memory[0] =	1;
	memory[1] =	3;

	strcpy(sigcfg_master.product_type, "BreweryMonitor");
	sigcfg_master.num_reports =	REPORT_GROUPS;

	uint16_t reg_addr_temperature =	1500;
	uint16_t reg_addr_temperaturesp	= 1501;
	uint16_t reg_addr_coilstatus = 1101;

	for	(int grp = 0 ; grp < sigcfg_master.num_reports ; grp++)	{

		strcpy(sigcfg_master.reports[grp].report_name, "TankStatistics");

		sigcfg_master.reports[grp].sample_interval_s = SENSOR_RATE;
		sigcfg_master.reports[grp].report_interval_s = REPORT_RATE;
		sigcfg_master.reports[grp].next_sample_time	= 0;
		sigcfg_master.reports[grp].max_records = MAX_RECORDS;
		sigcfg_master.reports[grp].storage[0].ts_s = ts_s[grp];
		sigcfg_master.reports[grp].storage[0].ts_ms	= ts_ms[grp];
		sigcfg_master.reports[grp].num_fields =	5;

		strcpy(sigcfg_master.reports[grp].fields[0].field_name,	"Temperature");
		sigcfg_master.reports[grp].fields[0].protocol =	MODBUS;
		sigcfg_master.reports[grp].fields[0].type =	CBOR_INT32;
		sigcfg_master.reports[grp].fields[0].storage[0].size = sizeof(field_data[0][0][0]);
		sigcfg_master.reports[grp].fields[0].storage[0].data = field_data[grp][0];
		sigcfg_master.reports[grp].fields[0].num_actions = 1;
		sigcfg_master.reports[grp].fields[0].action[0].modbus.slave_addr = DEVICE_ADDRESS;
		sigcfg_master.reports[grp].fields[0].action[0].modbus.opcode = 0x03;
		sigcfg_master.reports[grp].fields[0].action[0].modbus.reg_addr = reg_addr_temperature;
		sigcfg_master.reports[grp].fields[0].action[0].modbus.num =	1;
		sigcfg_master.reports[grp].fields[0].action[0].modbus.storage.data = sigcfg_master.reports[grp].fields[0].storage[0].data;
		sigcfg_master.reports[grp].fields[0].action[0].modbus.storage.size = sigcfg_master.reports[grp].fields[0].storage[0].size;

		strcpy(sigcfg_master.reports[grp].fields[1].field_name,	"TemperatureUnit");
		sigcfg_master.reports[grp].fields[1].protocol =	MEMORY;
		sigcfg_master.reports[grp].fields[1].type =	CBOR_INT32;
		sigcfg_master.reports[grp].fields[1].storage[0].size = sizeof(field_data[0][0][0]);
		sigcfg_master.reports[grp].fields[1].storage[0].data = field_data[grp][1];
		sigcfg_master.reports[grp].fields[1].num_actions = 1;
		sigcfg_master.reports[grp].fields[1].action[0].memory.reg_addr = 0x400;
		sigcfg_master.reports[grp].fields[1].action[0].memory.storage.data = sigcfg_master.reports[grp].fields[1].storage[0].data;
		sigcfg_master.reports[grp].fields[1].action[0].memory.storage.size = sigcfg_master.reports[grp].fields[1].storage[0].size;

		strcpy(sigcfg_master.reports[grp].fields[2].field_name,	"Slot");
		sigcfg_master.reports[grp].fields[2].protocol =	MEMORY;
		sigcfg_master.reports[grp].fields[2].type =	CBOR_INT32;
		sigcfg_master.reports[grp].fields[2].storage[0].size = sizeof(field_data[0][0][0]);
		sigcfg_master.reports[grp].fields[2].storage[0].data = field_data[grp][2];
		sigcfg_master.reports[grp].fields[2].num_actions = 1;
		sigcfg_master.reports[grp].fields[2].action[0].memory.reg_addr = 0x401 + grp;
		sigcfg_master.reports[grp].fields[2].action[0].memory.storage.data = sigcfg_master.reports[grp].fields[2].storage[0].data;
		sigcfg_master.reports[grp].fields[2].action[0].memory.storage.size = sigcfg_master.reports[grp].fields[2].storage[0].size;

		strcpy(sigcfg_master.reports[grp].fields[3].field_name,	"TemperatureSetPoint");
		sigcfg_master.reports[grp].fields[3].protocol =	MODBUS;
		sigcfg_master.reports[grp].fields[3].type =	CBOR_INT32;
		sigcfg_master.reports[grp].fields[3].storage[0].size = sizeof(field_data[0][0][0]);
		sigcfg_master.reports[grp].fields[3].storage[0].data = field_data[grp][3];
		sigcfg_master.reports[grp].fields[3].num_actions = 1;
		sigcfg_master.reports[grp].fields[3].action[0].modbus.slave_addr = DEVICE_ADDRESS;
		sigcfg_master.reports[grp].fields[3].action[0].modbus.opcode = 0x03;
		sigcfg_master.reports[grp].fields[3].action[0].modbus.reg_addr = reg_addr_temperaturesp;
		sigcfg_master.reports[grp].fields[3].action[0].modbus.num =	1;
		sigcfg_master.reports[grp].fields[3].action[0].modbus.storage.data = sigcfg_master.reports[grp].fields[3].storage[0].data;
		sigcfg_master.reports[grp].fields[3].action[0].modbus.storage.size = sigcfg_master.reports[grp].fields[3].storage[0].size;

		strcpy(sigcfg_master.reports[grp].fields[4].field_name,	"CoilStatus");
		sigcfg_master.reports[grp].fields[4].protocol =	MODBUS;
		sigcfg_master.reports[grp].fields[4].type =	CBOR_INT32;
		sigcfg_master.reports[grp].fields[4].storage[0].size = sizeof(field_data[0][0][0]);
		sigcfg_master.reports[grp].fields[4].storage[0].data = field_data[grp][4];
		sigcfg_master.reports[grp].fields[4].num_actions = 1;
		sigcfg_master.reports[grp].fields[4].action[0].modbus.slave_addr = DEVICE_ADDRESS;
		sigcfg_master.reports[grp].fields[4].action[0].modbus.opcode = 0x01;
		sigcfg_master.reports[grp].fields[4].action[0].modbus.reg_addr = reg_addr_coilstatus;
		sigcfg_master.reports[grp].fields[4].action[0].modbus.num =	16;
		sigcfg_master.reports[grp].fields[4].action[0].modbus.storage.data = sigcfg_master.reports[grp].fields[4].storage[0].data;
		sigcfg_master.reports[grp].fields[4].action[0].modbus.storage.size = sigcfg_master.reports[grp].fields[4].storage[0].size;

		reg_addr_temperature +=	10;
		reg_addr_temperaturesp += 10;
		reg_addr_coilstatus	+= 1;
	}
#endif

#if	0
	memory[0] =	1;
	memory[1] =	3;

	// TODO(SIGCELL-25,David): Set these to	an empty default value?

	strcpy(sigcfg_master.product_type, "TempHumidLightSensor");
	sigcfg_master.num_reports =	REPORT_GROUPS;

	uint8_t	reg_addr_temperature = 0xf3;
	uint8_t	reg_addr_humidity =	0xf5;

	for	(int grp = 0 ; grp < sigcfg_master.num_reports ; grp++)	{

		strcpy(sigcfg_master.reports[grp].report_name, "THL");

		sigcfg_master.reports[grp].sample_interval_s = SENSOR_RATE;
		sigcfg_master.reports[grp].report_interval_s = REPORT_RATE;
		sigcfg_master.reports[grp].next_sample_time	= 0;
		sigcfg_master.reports[grp].max_records = MAX_RECORDS;
		sigcfg_master.reports[grp].storage[0].ts_s = ts_s[grp];
		sigcfg_master.reports[grp].storage[0].ts_ms	= ts_ms[grp];
		sigcfg_master.reports[grp].num_fields =	1;

		strcpy(sigcfg_master.reports[grp].fields[0].field_name,	"Temperature");
		sigcfg_master.reports[grp].fields[0].protocol =	I2C;
		sigcfg_master.reports[grp].fields[0].type =	CBOR_INT32;
		sigcfg_master.reports[grp].fields[0].storage[0].size = sizeof(field_data[0][0][0]);
		sigcfg_master.reports[grp].fields[0].storage[0].data = field_data[grp][0];
		sigcfg_master.reports[grp].fields[0].num_actions = 1;
		sigcfg_master.reports[grp].fields[0].action[0].i2c.slave_addr =	0x40;
		sigcfg_master.reports[grp].fields[0].action[0].i2c.bus_name	= "I2C_2";
		sigcfg_master.reports[grp].fields[0].action[0].i2c.reg_addr	= reg_addr_temperature;
		sigcfg_master.reports[grp].fields[0].action[0].i2c.wlen	= 1;
		sigcfg_master.reports[grp].fields[0].action[0].i2c.rlen	= 3;
		sigcfg_master.reports[grp].fields[0].action[0].i2c.storage.data	= sigcfg_master.reports[grp].fields[0].storage[0].data;
		sigcfg_master.reports[grp].fields[0].action[0].i2c.storage.size	= sigcfg_master.reports[grp].fields[0].storage[0].size;
	}
#endif

#ifndef	CONFIG_SENSOR_NONE
	// TODO(David,SIGCELL-): Replace this with a less hacky	way	to check, and setup	structure for, sensors.
	sigcfg_master.reports[0].fields[0].protocol	= I2C;	// Allows the first	perform_action() call to initialize	structure for sensors.
#endif
	sigcfg_master.num_reports =	VAR_MAX_REPORTS;
	sigcfg_master.sequence = 1;
	strcpy(sigcfg_master.product_type, var_devtype.data);

	for	(int grp = 0 ; grp < sigcfg_master.num_reports ; grp++)	{

		sigcfg_master.reports[grp].max_records = VAR_MAX_REPORTS;
		sigcfg_master.reports[grp].num_fields =	VAR_MAX_FIELDS;
		strcpy(sigcfg_master.reports[grp].report_name, var_report[grp].data);

		for	(int q = 0 ; q < VAR_REPORT_QUEUE_SIZE ; q++) {
			sigcfg_master.reports[grp].storage[q].ts_s = ts_s[grp][q];
			sigcfg_master.reports[grp].storage[q].ts_ms	= ts_ms[grp][q];

			for	(int f = 0 ; f < VAR_MAX_FIELDS	; f++) {
				sigcfg_master.reports[grp].fields[f].storage[q].size = (uint8_t)sizeof(field_data[grp][f][q]);
				sigcfg_master.reports[grp].fields[f].storage[q].data = field_data[grp][f][q];
				sigcfg_master.reports[grp].fields[f].num_actions = 0;
			}
		}
	}
}

void* sigconfig_set_report_names(void *value)
{
	for	(int grp = 0 ; grp < sigcfg_master.num_reports ; grp++)
		strcpy(sigcfg_master.reports[grp].report_name, var_report[grp].data);
	return NULL;
}

/*
u8_t make_i2c_transaction(int index, struct	i2c_action_s * action)
{
	struct device *dev = device_get_binding(action->bus_name);
	if (dev) {
		u8_t readVal[3]	= {0};
		u8_t sendbuf[2]	= {0};
		int	i;

		sendbuf[0] = action->reg_addr;

		if (i2c_write(dev, sendbuf,	action->wlen, action->slave_addr) >= 0)	{
		}
	while (1) {	// TODO: Change	to 1 after I2C working
		k_sleep(100);
		if (i2c_read(dev, readVal, action->rlen, action->slave_addr) ==	0) {
			break;
		}
		k_sleep(100); // TODO: add timeout
	}
	action->storage.data[0]	= 0;
	for	( i	= 0	; i	< action->rlen ; i++ ) {
//		action->storage.data[0]	= (action->storage.data[0] << 8) | readVal[i];
		action->storage.data[i]	= readVal[i];
	}
}

return 0;
}
*/
bool sigconfig_execute(int64_t current_time, int64_t real_time)
{
	bool sigconfig_data_ready =	false;

	for	(int grp = 0; grp <	sigcfg_master.num_reports; grp++)
	{
		// Check report	queue is free to write to
		if (k_sem_take(&sem_report_queue, K_MSEC(100)) != 0) {
			LOG_ERR("Group %d queue	busy. Data dropped.", grp);
			uart_send("+notify,queue:busy\r\n",	0);
			return sigconfig_data_ready;
		}

		int	index =	sigcfg_master.reports[grp].index;
		if (index <	VAR_MAX_REPORTS) {
			if ((sigcfg_master.reports[grp].next_sample_time * 1000) <=	current_time)
			{
				LOG_INF("Performing	action for group %d", grp);
				sigcfg_master.reports[grp].storage[index].ts_s = (uint32_t)(real_time /	1000);
				sigcfg_master.reports[grp].storage[index].ts_ms	= (uint32_t)(real_time % 1000);
				perform_actions(index, &sigcfg_master.reports[grp]);
				sigcfg_master.reports[grp].next_sample_time	= (current_time	/ 1000)	+ sigcfg_master.reports[grp].sample_interval_s;
				sigcfg_master.reports[grp].index++;
				sigcfg_master.reports[grp].count++;
				sigconfig_data_ready = true;
			}
		}

		k_sem_give(&sem_report_queue);
	}

	return sigconfig_data_ready;
}

uint32_t sigconfig_push(char *report, struct var_param_s *params, int param_count, int64_t real_time)
{
	uint32_t sequence =	0;

	// For all report groups...
	for	(int grp = 0; grp <	sigcfg_master.num_reports; grp++)
	{
		// Verify report name
		if (strcmp(report, sigcfg_master.reports[grp].report_name) == 0) {

			// Verify availability in report queue
			int	count =	sigcfg_master.reports[grp].count;
			if (count <	VAR_REPORT_QUEUE_SIZE) {

				// Check report	queue is free to write to
				if (k_sem_take(&sem_report_queue, K_MSEC(100)) != 0) {
					LOG_ERR("Group %d [%s] queue busy. Data	dropped.", grp,	log_strdup(report));
					uart_send("+rsp,push:busy\r\n",	0);
					return 0;
				}

				// Set report queue	index to next avaliable	element	in array.
				int	index =	(sigcfg_master.reports[grp].index %	VAR_REPORT_QUEUE_SIZE);
				LOG_INF("Storing data for group	%d [%s]	in element %d of queue.", grp, log_strdup(report), index);

				sigcfg_master.reports[grp].storage[index].ts_s = (uint32_t)(real_time /	1000);
				sigcfg_master.reports[grp].storage[index].ts_ms	= (uint32_t)(real_time % 1000);

				// Limit report	fields to max
				int	no_params =	param_count;
				if (no_params >	VAR_MAX_FIELDS)	{
					LOG_ERR("Unable	to parse more than %d fields.",	VAR_MAX_FIELDS);
					no_params =	VAR_MAX_FIELDS;
				}

				// Copy	field data to report fields
				for	(int field = 0 ; field < no_params ; field++) {
					if (params[field].value) {
						if (params[field].value[0] == '"') {
							sigcfg_master.reports[grp].fields[field].type =	CBOR_TEXT;
							params[field].value[strlen(params[field].value)] = 0;
							strncpy((char*)sigcfg_master.reports[grp].fields[field].storage[index].data, &params[field].value[0], sigcfg_master.reports[grp].fields[field].storage[index].size);
						}
						else {
							sigcfg_master.reports[grp].fields[field].type =	CBOR_INT32;
							*((uint32_t*)(sigcfg_master.reports[grp].fields[field].storage[index].data)) = atoi(params[field].value);
						}
					}
					else {
						sigcfg_master.reports[grp].fields[field].type =	CBOR_TEXT;
						sigcfg_master.reports[grp].fields[field].storage[index].data[0]	= 0;
					}

					strncpy(sigcfg_master.reports[grp].fields[field].field_name, params[field].key,	sizeof(sigcfg_master.reports[grp].fields[field].field_name));
				}

				sigcfg_master.reports[grp].num_fields =	no_params;

				// Update sequence number
				sequence = sigcfg_master.sequence++;
				sigcfg_master.reports[grp].storage[index].sequence = sequence;

				// Update index	and	count
				sigcfg_master.reports[grp].index++;
				sigcfg_master.reports[grp].count++;
				sigconfig_data_ready = true;

				k_sem_give(&sem_report_queue);

				// LORA	LTE_send();
				break;
			}
			else {
				LOG_ERR("Group %d [%s] queue full. Data	dropped.", grp,	log_strdup(report));
				uart_send("+rsp,push:queue_full\r\n", 0);
			}
		}
	}

	return sequence;
}

bool sigconfig_ready_to_send(void)
{
	for	(int grp = 0; grp <	sigcfg_master.num_reports; grp++)
	{
		int	reporting_interval = sigcfg_master.reports[grp].report_interval_s /	sigcfg_master.reports[grp].sample_interval_s;
		if (sigcfg_master.reports[grp].count >=	reporting_interval)	{
			return true;
		}
	}
	return false;
}

bool sigconfig_full(void)
{
	for	(int grp = 0; grp <	sigcfg_master.num_reports; grp++)
	{
		if (sigcfg_master.reports[grp].count >=	sigcfg_master.reports[grp].max_records)	{
			return true;
		}
	}
	return false;
}

bool sigconfig_get_records_to_send(int count, int idx, int *record_first, int *record_last)
{
	// Check that records exist	in queue
	if(count){
		// Set indexed range of	records
		*record_last = idx % VAR_REPORT_QUEUE_SIZE - 1;
		*record_first =	*record_last - count+1;
		if (*record_first <	0) {
			*record_first =	*record_first +	VAR_REPORT_QUEUE_SIZE;
		}

		return true;
	}
	return false;
}

static int process_server_message(uint8_t *pCbor, uint32_t cborLen);
#if	defined(SUPPORT_SSR)
static int process_ssr(cbor_data_t server_request);
static void	ssr_process(cbor_data_t	c);
#endif
static int get_sigconfig_settings(cbor_data_t server_request, struct sigconfig_master_s	* Settings);
static int queue_ssr_reply(uint32_t	ssrid, const char *reply);
static uint8_t get_field_type_from_text(uint8_t	* type);
static uint8_t get_opcode_from_text(uint8_t	* reg_type);
static uint8_t get_protocol_from_text(uint8_t *	proto);

void sigconfig_process_cbor_payload(uint8_t	*payload, uint32_t length)
{
	// printf(<93>Received Rx. CoAP/CBOR payload length	%d.\n<94>, length);
	// DPRINT_frame_hex(payload, length);
	uint8_t	*pCoapCbor = payload;
	uint32_t len_coap_cbor = length;

	// unpack CoAP/CBOR	to get CBOR	payload
	static uint8_t cbor[1024]; //

	uint8_t	*pCbor = &cbor[0];
	uint32_t cborLen;

	extractPayloadFromCoapPDU(pCoapCbor, len_coap_cbor,	(uint8_t **)&pCbor,	(size_t	*)&cborLen);
	// printf(<93>Rx payload: CBOR length %d.\n<94>, cborLen);
	// DPRINT_frame_hex(pCbor, cborLen);

	if (cborLen	== 0) {
		return;
	}

	process_server_message(pCbor, cborLen);

	return;
}

static int process_server_message(uint8_t *pCbor, uint32_t cborLen)
{
	cbor_data_t	server_request;
	server_request.b = pCbor;
	server_request.len = cborLen;

#if	defined(SUPPORT_SSR)
	process_ssr(server_request);
#endif
	return 0;

}

#if	defined(SUPPORT_SSR)
static int process_ssr(cbor_data_t server_request)
{
	cbor_data_t	cur	= cbor_retrieve(server_request,	"ssr");
	if (cur.b == NULL)
	{
		return 0;
	}
	int	num_ssr	= cbor_get_array_count(&cur);
	ssr_process(cur);

	return 1;
}
#endif

#define	SERV_COMMAND_SET_PARAM		"set_param"
#define	SERV_COMMAND_SET_SIGCFG		"set_device_config"
#define	MAX_SERV_COMMAND_STRLEN	64

#if	defined(SUPPORT_SSR)
static void	ssr_process(cbor_data_t	c)
{
	char type[MAX_SERV_COMMAND_STRLEN];
	uint32_t ssrid;

	cbor_retrieve_text(c, "type", type,	sizeof(type), "");
	ssrid =	cbor_retrieve_int(c, "id", 0);
	if(!strncmp(type, SERV_COMMAND_SET_SIGCFG, strlen(SERV_COMMAND_SET_SIGCFG))) {
		get_sigconfig_settings(c, &sigcfg_master2);
		queue_ssr_reply(ssrid, "success");
		sigcfg_master.reports[0].sample_interval_s = 600;
		need_settings =	false;
	}
	return;

}
#endif

static int queue_ssr_reply(uint32_t	ssrid, const char *reply)
{
#if	0
	ssr_pending	= 1;
	strncpy(ssr_status,	reply, sizeof(ssr_status));
	ssr_id = ssrid ;
#endif

	return 1;
}

static int get_sigconfig_settings(cbor_data_t server_request, struct sigconfig_master_s	* Settings)
{
	char name[16];
	char proto[16];
	char port[16];
	int	out;
	int	addr;
	int	nums;
	char reg_type[16];
	int	reg;
	char ports_parity[2];
	char type[16];
	char temp_buff [16];
	char outValue [64];
	struct sigconfig_master_s settings;
	uint8_t	* packet_space = &sigcfg_packet_space;

	// cbor_data_t server_request;
	// server_request.b	= cbor2;
	// server_request.len =	 sizeof(cbor2);

	memset(&settings, 0, sizeof(settings));
	//cbor_data_t cb_ssr = cbor_retrieve(server_request, "ssr");
	//int cb_num_ssr = cbor_get_array_count(&cb_ssr);
	cbor_data_t	cb_ssr_data	= cbor_retrieve(server_request,	"data");
	cbor_data_t	cb_ssr_data_ports =	cbor_retrieve(cb_ssr_data, "ports");


	cbor_data_t	cb_ssr_data_reports	= cbor_retrieve(cb_ssr_data, "reports_types");

	int	num_data_ports = cbor_get_array_count(&cb_ssr_data_ports);
	settings.num_reports = cbor_get_array_count(&cb_ssr_data_reports);


	int	baud = cbor_retrieve_int(cb_ssr_data_ports,	"baud",	0);
	cbor_retrieve_text(cb_ssr_data_ports, "name", name,	sizeof(name), "");
	cbor_retrieve_text(cb_ssr_data_ports, "parity",	ports_parity, sizeof(ports_parity),	"");
	int	stopbit	= cbor_retrieve_int(cb_ssr_data_ports, "stopbit", 0);

	cbor_retrieve_text(cb_ssr_data_ports, "type", type,	sizeof(type), "");

	// printf("	 name:%s, type:%s, baud:%d,	parity:%s, stopbit:%d \n",
	//	name, type,	baud, ports_parity,	stopbit);





	///	parsing	reports
	for(size_t i = 0; i	< settings.num_reports;	i++)
	{
		settings.reports[i].report_interval_s =	cbor_retrieve_int(cb_ssr_data_reports, "report_interval", 0);
		settings.reports[i].sample_interval_s =	cbor_retrieve_int(cb_ssr_data_reports, "sample_interval", 0);
		cbor_retrieve_text(cb_ssr_data_reports,	"name",	&settings.reports[i].report_name, sizeof(settings.reports[i].report_name), "");

		// printf("	 name:%s, reports_types_report_interval:%d,	reports_types_sample_interval:%d, \n",
		//	name, reports_types_report_interval, reports_types_sample_interval);




		cbor_data_t	cb_ssr_data_reports_fields = cbor_retrieve(cb_ssr_data_reports,	"fields");
		settings.reports[i].num_fields = cbor_get_array_count(&cb_ssr_data_reports_fields);


		bool valid = true;

		for	(size_t	j =	0; j < settings.reports[i].num_fields && valid;	j++)
		{

			cbor_retrieve_text(cb_ssr_data_reports_fields, "name", settings.reports[i].fields[j].field_name, sizeof(settings.reports[i].fields[j].field_name), "");
			cbor_retrieve_text(cb_ssr_data_reports_fields, "outValue", outValue, sizeof(outValue), "");
			cbor_retrieve_text(cb_ssr_data_reports_fields, "type", temp_buff, sizeof(temp_buff), "");

			settings.reports[i].fields[j].type = get_field_type_from_text(temp_buff);
			// printf("	 name:%s, outValue:%s, type:%s \n",	name, outValue,	type);
			for	(int q = 0 ; q < VAR_REPORT_QUEUE_SIZE ; q++) {
				if(settings.reports[i].fields[j].type == CBOR_INT32)
				{
					settings.reports[i].fields[j].storage[q].data =	(uint32_t*)packet_space;
					settings.reports[i].fields[j].storage[q].size =	4;
					packet_space +=	4;
				}
			}

			cbor_data_t	cb_ssr_data_reports_fields_get = cbor_retrieve(cb_ssr_data_reports_fields, "get");
			settings.reports[i].fields[j].num_actions =	cbor_get_array_count(&cb_ssr_data_reports_fields_get);


			for(size_t k = 0; k	< settings.reports[i].fields[j].num_actions; k++)
			{
				int	out	= cbor_retrieve_int(cb_ssr_data_reports_fields_get,	"out", 0);
				cbor_retrieve_text(cb_ssr_data_reports_fields_get, "port", port, sizeof(port), "");
				cbor_retrieve_text(cb_ssr_data_reports_fields_get, "proto",	temp_buff, sizeof(temp_buff), "");
				settings.reports[i].fields[j].protocol = get_protocol_from_text(temp_buff);
				// printf("		out:%d,	port:%s, proto:%s \n", out,	port, proto);

				cbor_data_t	cb_ssr_data_reports_fields_get_operation = cbor_retrieve(cb_ssr_data_reports_fields_get, "operation");
				// int num_get = cbor_get_array_count(&cb_ssr_data_reports_fields_get);

				settings.reports[i].fields[j].action[k].modbus.slave_addr =	cbor_retrieve_int(cb_ssr_data_reports_fields_get_operation,	"addr",0);
				settings.reports[i].fields[j].action[k].modbus.num = cbor_retrieve_int(cb_ssr_data_reports_fields_get_operation, "nums",0);
				settings.reports[i].fields[j].action[k].modbus.reg_addr	= cbor_retrieve_int(cb_ssr_data_reports_fields_get_operation, "reg",0);
				cbor_retrieve_text(cb_ssr_data_reports_fields_get_operation, "reg_type", temp_buff,	sizeof(temp_buff), "");
				settings.reports[i].fields[j].action[k].modbus.opcode =	get_opcode_from_text(temp_buff);
				// TODO: Confirm next 3	lines
				settings.reports[i].fields[j].action[k].modbus.storage.data	= (uint32_t*)packet_space;
				settings.reports[i].fields[j].action[k].modbus.storage.size	= settings.reports[i].fields[j].action[k].modbus.num;
				packet_space +=settings.reports[i].fields[j].action[k].modbus.num;
				// printf("			 addr:%d, nums:%d, reg:%d, reg_type:%s \n",	addr, nums,	reg, reg_type);
			}
			valid =	cbor_skip(&cb_ssr_data_reports_fields);
		}
	}
	*Settings =	settings;
	return 0;
}

static uint8_t get_field_type_from_text(uint8_t	* type)
{
	if(strstr(type,	"Integer"))
		return CBOR_INT32;
	if(strstr(type,	"Float"))
		return CBOR_FLOAT_REVERSE;	// I've	always had to reverse the floats from IoT board, but some device might give	it reversed	already.
	if(strstr(type,	"Double"))
		return CBOR_DOUBLE;			// Last	time double	was	attempted, it did not work.
	if(strstr(type,	"Boolean"))
		return CBOR_BOOL;
	if(strstr(type,	"Text"))
		return CBOR_TEXT;
	// TODO	Not	sure what to do	about DateTime.
	return CBOR_ERROR;
}

static uint8_t get_opcode_from_text(uint8_t	* reg_type)
{
	if(strstr(reg_type,	"HOLDING"))
		return 0x03;
	return 0;
	// TODO	find out what Dima will	send down for coils	and	such and parse those as	well
}

static uint8_t get_protocol_from_text(uint8_t *	proto)
{
	if(strstr(proto, "MODBUS-RTU"))
		return MODBUS;
	return PROTO_ERROR;
}
