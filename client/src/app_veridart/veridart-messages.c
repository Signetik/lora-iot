//============================================================================//
//                                                                            //
//        .d8888b.  d8b                            888    d8b 888             //
//       d88P  Y88b Y8P                            888    Y8P 888             //
//       Y88b.                                     888        888             //
//        "Y888b.   888  .d88b.  88888b.   .d88b.  888888 888 888  888        //
//           "Y88b. 888 d88P"88b 888 "88b d8P  Y8b 888    888 888 .88P        //
//             "888 888 888  888 888  888 88888888 888    888 888888K         //
//       Y88b  d88P 888 Y88b 888 888  888 Y8b.     Y88b.  888 888 "88b        //
//        "Y8888P"  888  "Y88888 888  888  "Y8888   "Y888 888 888  888        //
//                           888                                              //
//                      Y8b d88P                                              //
//                       "Y88P"                                Signetik, LLC  //
//                                                           www.signetik.com //
//----------------------------------------------------------------------------//
//           Copyright © 2020 Signetik, LLC -- All Rights Reserved            //
//   Signetik Confidential Proprietary Information -- Disclosure Prohibited    //
//----------------------------------------------------------------------------//
// Project   : SafeTraces veriDART                                            //
// Filename  :                                                                //
// Author(s) :                                                                //
// Created   :                                                                //
// Purpose   :                                                                //
//============================================================================//
// NOTE: Signetik will transfer ownership to SafeTraces on final delivery.

/*
 * veridart_messages.c
 *
 * Created: 8/14/2020 10:42:34 AM
 *  Author: Remy Patterson
 */ 

#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <logging/log.h>

#include "veridart-messages.h"
#include <time.h>
#include "main.h"
#include "timing_profile.h"
#include "../LoRa_task.h"
#include "veridart_main_task.h"
#include "led_control.h"
#include "utility.h"
#include "../version.h"

static uint32_t last_uplink_queue_time_sec = 0;

#define SEND_VERSIONNUM_TO_SERVER
//#define SEND_RESETCAUSE_TO_SERVER

LOG_MODULE_REGISTER(veridartmsg, CONFIG_SIGNETIK_CLIENT_LOG_LEVEL);

void rtc_match_callback(void)
{
#ifdef STEVE
	rtc_calendar_disable_callback(&rtc_instance, RTC_CALENDAR_CALLBACK_ALARM_0);
#endif
	k_sem_give(&execution_start_semaphore);
}

#define GRACE_PERIOD_FOR_TIMER_SETUP_TO_TRIGGER_ALARM 5
#define FORGIVENESS_TIME_FOR_DELAYED_GO 60

//This function will get the start time from the server 'go' packet and set the RTC callback to happen at that time
static void set_rtc_callback_from_packet(uint8_t* packet)
{
	uint32_t time_epochsec2020 = 0;
	 
	time_epochsec2020 |= packet[1] << 24;
	time_epochsec2020 |= packet[2] << 16;
	time_epochsec2020 |= packet[3] << 8;
	time_epochsec2020 |= packet[4];
	
	uint32_t time_epochsec2020_now = get_rtc_time();
	LOG_DBG("go time is %d secs from now", time_epochsec2020 - time_epochsec2020_now);
	int time_to_go_secs = time_epochsec2020 - time_epochsec2020_now;
 	
#ifdef STEVE
	struct rtc_calendar_time t;

	

	//If GO time is past and was in the immediate XX seconds, start the operation immediately. 
		//This is to account for not getting GO in time due to network congestion.
	if((time_to_go_secs < 0) && (time_to_go_secs > -FORGIVENESS_TIME_FOR_DELAYED_GO)){
		LOG_DBG("GO time is past but within forgiveness window, execution will be started!");				
		epochsec2020_to_calendar_time(time_epochsec2020_now+GRACE_PERIOD_FOR_TIMER_SETUP_TO_TRIGGER_ALARM, &t);
	}
	else {
		epochsec2020_to_calendar_time(time_epochsec2020, &t);
	}
	
	LOG_DBG("Timing profile will start executing at: %4d%02d%02d %02d:%02d:%02d", t.year, t.month, t.day, t.hour, t.minute, t.second);

	struct rtc_calendar_alarm_time alarm;
	alarm.mask = RTC_CALENDAR_ALARM_MASK_YEAR;
	alarm.time.year = t.year;
	alarm.time.month =  t.month;
	alarm.time.day = t.day;
	alarm.time.hour = t.hour;
	alarm.time.minute = t.minute;
	alarm.time.second = t.second;
	rtc_calendar_set_alarm(&rtc_instance, &alarm, RTC_CALENDAR_ALARM_0);
	rtc_calendar_enable_callback(&rtc_instance, RTC_CALENDAR_CALLBACK_ALARM_0);
#endif
}

static void set_profile_from_packet(uint8_t* pkt)
{
	timingprofile_t profile;
	memset(&profile, 0, sizeof(profile));
	
	profile.pre_delay_sec |= pkt[1] << 8;
	profile.pre_delay_sec |= pkt[2];
	
	profile.on_msec |= pkt[3] << 24;
	profile.on_msec |= pkt[4] << 16;
	profile.on_msec |= pkt[5] << 8;
	profile.on_msec |= pkt[6];
	
	profile.number_of_sprays |= pkt[7] << 8;
	profile.number_of_sprays |= pkt[8];
	
	profile.off_sec |= pkt[9] << 8;
	profile.off_sec |= pkt[10];
	
	timing_profile_set(&profile);
}

static void process_packet_setclock(uint8_t* packet)
{
	uint32_t time_epochsec2020 = 0;
		
	uint8_t current_state = get_device_state();
	
	if ((current_state == device_booted)||
		(current_state == device_timesetup))			
	{
		time_epochsec2020 |= packet[1] << 24;
		time_epochsec2020 |= packet[2] << 16;
		time_epochsec2020 |= packet[3] << 8;
		time_epochsec2020 |= packet[4];
		set_rtc(time_epochsec2020);
		
		set_device_state(device_timesetup);
		set_device_tx_quiet(0);
		led_on();
	}
}

static void process_packet_settimingprofile(uint8_t* packet)
{
	uint8_t current_state = get_device_state();
	
	if ((current_state == device_timesetup) || (current_state == device_operation_stopped) || (current_state == device_obtained_timing_profile))
	{
		set_profile_from_packet(packet);
			
		set_device_state(device_obtained_timing_profile);	
		set_device_tx_quiet(0);
	}
	
}

static void process_packet_go(uint8_t* packet)
{
	uint8_t current_state = get_device_state();
	
	if ((current_state == device_obtained_timing_profile) || (current_state == device_operation_stopped))
	{
		set_device_state(device_obtained_go);
		set_device_tx_quiet(0);

		set_rtc_callback_from_packet(packet);

	}
}

static void process_packet_stop()
{
	uint8_t current_state = get_device_state();
	
	if ((current_state == device_start_delay) || (current_state == device_active_start) || (current_state == device_active_end))
	{
		LOG_DBG("Stopping current operation");
		k_sem_give(&cancel_operation_semaphore);
	}
}

//ACKs from server



static void hex_dump(const void *src, size_t length, size_t line_size,
const char *prefix)
{
	int i = 0;
	const unsigned char *address = (const unsigned char *)src;
	const unsigned char *line = (const unsigned char *)address;
	unsigned char c;
	static char buffer[255];
	static char temp[64];
	int buffer_len = 0;

	snprintf(temp, sizeof(temp), "%s | ", prefix);
	strncpy(buffer, temp, sizeof(buffer));
	buffer_len += strlen(temp);
	while (length-- > 0) {
		snprintf(temp, sizeof(temp), "%02X ", *address++);
		strncat(buffer, temp, sizeof(buffer)-buffer_len);
		buffer_len += strlen(temp);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size) {
					snprintf(temp, sizeof(temp),  "__ ");
					strncat(buffer, temp, sizeof(buffer)-buffer_len);
					buffer_len += strlen(temp);
				}
			}
			snprintf(temp, sizeof(temp),  " | ");  /* right close */
			strncat(buffer, temp, sizeof(buffer)-buffer_len);
			buffer_len += strlen(temp);
			while (line < address) {
				c = *line++;
				snprintf(temp, sizeof(temp),  "%c", (c < 33 || c == 255) ? 0x2E : c);
				strncat(buffer, temp, sizeof(buffer)-buffer_len);
				buffer_len += strlen(temp);
			}
			snprintf(temp, sizeof(temp),  "\n");
			strncat(buffer, temp, sizeof(buffer)-buffer_len);
			buffer_len += strlen(temp);
			if (length > 0) {
				snprintf(temp, sizeof(temp),  "%s | ", prefix);
				strncat(buffer, temp, sizeof(buffer)-buffer_len);
				buffer_len += strlen(temp);
			}
		}
	}
	LOG_DBG("%s ",buffer);
}

void veridart_process_downlink(uint8_t* packet, uint8_t packetLen)
{
	uint8_t data [64];
	
	memset(data, 0, sizeof(data));
	memcpy (data, packet, packetLen);
	hex_dump(packet, packetLen, packetLen, "RX");
	uint8_t current_state = get_device_state();
	
	switch(data[0])
	{
		case pkt_server_bootupACK:
			LOG_DBG("got packet server_bootupACK");
			set_device_tx_quiet(0);
			break;
		case pkt_server_setclock :
			LOG_DBG("got packet server_setclock");
			process_packet_setclock(packet);
			set_device_tx_quiet(0);
			break;
		case pkt_server_timesetupACK:
			LOG_DBG("got packet server_timesetupACK");
			if(current_state == device_timesetup) { set_device_tx_quiet(1); }
			break;
		case pkt_server_settimingprofile :
			LOG_DBG("got packet server_settimingprofile");
			process_packet_settimingprofile(packet);
			set_device_tx_quiet(0);
			break;
		case pkt_server_timingprofileACK:
			LOG_DBG("got packet server_timingprofileACK");
			if(current_state == device_obtained_timing_profile) { set_device_tx_quiet(1); }
			break;
		case pkt_server_go :
			LOG_DBG("got packet server_go");
			process_packet_go(packet);
			break;
		case pkt_server_goACK:
			LOG_DBG("got packet server_goACK");
			if(current_state == device_obtained_go) { set_device_tx_quiet(1); }
			break;
		case pkt_server_completedACK:
			LOG_DBG("got packet server_completedACK");
			if(current_state == device_operation_complete) { set_device_tx_quiet(1); }
			break;
		case pkt_server_stop :
			LOG_DBG("got packet server_stop");
			process_packet_stop();
			set_device_tx_quiet(1);
			break;
		default:
			break;
	}
}




//Pack all the bytes in the correct order, then put LoRa packet in queue
static void pack_and_send_lora_packet(device_status_t* s_packet)
{
	struct lora_tx_message msg;
	
	msg.message[0] = s_packet->pkt_type;
	msg.message[1] = s_packet->device_state;
	msg.message[2] = (s_packet->epochsecs_2020 >> 24) & 0x000000FF;
	msg.message[3] = (s_packet->epochsecs_2020 >> 16) & 0x000000FF;
	msg.message[4] = (s_packet->epochsecs_2020 >> 8) & 0x000000FF;
	msg.message[5] = s_packet->epochsecs_2020 & 0x000000FF;
	msg.message[6] = (s_packet->crc16_tp_data >> 8) & 0x00FF;
	msg.message[7] = s_packet->crc16_tp_data & 0x00FF;
	msg.message[8] = s_packet->op_data.relay_active;
	msg.message[9] = (s_packet->op_data.completed_iterations >> 8) & 0x00FF;
	msg.message[10] = s_packet->op_data.completed_iterations & 0x00FF;
	msg.message[11] = s_packet->op_result;
	msg.length=12;
	
	k_msgq_put(&lora_tx_queue, &msg, K_NO_WAIT);
}

static int pack_tp_data(timingprofile_t* tp_data, char *packed)
{
	int i=0;
	packed[i++] = (tp_data->pre_delay_sec >> 8) & 0x00FF;
	packed[i++] = tp_data->pre_delay_sec & 0x00FF;
	packed[i++] = (tp_data->on_msec >> 24) & 0x000000FF;
	packed[i++] = (tp_data->on_msec >> 16) & 0x000000FF;
	packed[i++] = (tp_data->on_msec >> 8) & 0x000000FF;
	packed[i++] = tp_data->on_msec & 0x000000FF;
	packed[i++] = (tp_data->number_of_sprays >> 8) & 0x00FF;
	packed[i++] = tp_data->number_of_sprays & 0x00FF;
	packed[i++] = (tp_data->off_sec >> 8) & 0x00FF;
	packed[i++] = tp_data->off_sec & 0x00FF;
	return i;
}

static void queue_uplink(uint32_t queue_time_sec)
{
	device_status_t s_packet;
	uint8_t packet_type;
	
	uint8_t state = get_device_state();

	uint8_t tp_packed[32];
	uint8_t tp_packed_len;
	
	switch(state)
	{
		case device_booted :
			packet_type = pkt_device_status_booted;
			break;
		case device_timesetup :
			packet_type = pkt_device_timesetup;
			break;
		case device_obtained_timing_profile :
			packet_type = pkt_device_obtained_timing_profile;
			break;
		case device_obtained_go :
			packet_type = pkt_device_obtained_go;
			break;
		case device_start_delay :
			packet_type = pkt_device_start_delay;
			break;
		case device_active_start :
			packet_type = pkt_device_active_start;
			break;
		case device_active_end :
			packet_type = pkt_device_active_end;
			break;
		case device_operation_complete :
			packet_type = pkt_device_operation_complete;
			break;
		case device_operation_stopped :
			packet_type = pkt_device_operation_stopped;
			break;
		default:
			break;
	}
		
	timingprofile_t profile;
	timing_profile_get(&profile);
	tp_packed_len = pack_tp_data(&profile, tp_packed);
	uint16_t timing_profile_hash = crc16_veridart(tp_packed, tp_packed_len);

	operation_data_t op;
	timing_profile_get_operation_data(&op);
	
	uint8_t result = operation_status_get();
	
	s_packet.pkt_type = packet_type;
	s_packet.device_state = state;
	s_packet.epochsecs_2020 = queue_time_sec;
	s_packet.crc16_tp_data = timing_profile_hash;
	s_packet.op_data = op;
	s_packet.op_result = result;

#ifdef SEND_VERSIONNUM_TO_SERVER
	if(state == device_booted) {

	/*
	printf("enum system_reset_cause: %d %d %d %d %d %d %d ",
						SYSTEM_RESET_CAUSE_BACKUP,
						SYSTEM_RESET_CAUSE_SOFTWARE,
						SYSTEM_RESET_CAUSE_WDT,
						SYSTEM_RESET_CAUSE_EXTERNAL_RESET,
						SYSTEM_RESET_CAUSE_BOD33,
						SYSTEM_RESET_CAUSE_BOD12,
						SYSTEM_RESET_CAUSE_POR  );
	*/

		s_packet.crc16_tp_data = (VERSION_MAJOR & 0xF);
		s_packet.crc16_tp_data <<= 4;
		s_packet.crc16_tp_data |= (VERSION_MINOR & 0xF);
		s_packet.crc16_tp_data <<= 4;
		s_packet.crc16_tp_data |= (VERSION_INCREMENTAL & 0xF);
		s_packet.crc16_tp_data <<= 4;
		s_packet.crc16_tp_data |= (VERSION_ENGG & 0xF);
		LOG_DBG("Sending version number to server x%04x", s_packet.crc16_tp_data);
	}	
#endif
#ifdef SEND_RESETCAUSE_TO_SERVER
	#include <reset.h>
	if(state == device_booted) {
		//debug. Send reset_cause to server in this unused field for booted message
		s_packet.op_result = system_get_reset_cause();
		LOG_DBG("Sending reset_cause to server x%x", s_packet.op_result);
	}
#endif	
	
	pack_and_send_lora_packet(&s_packet);

}


void veridart_queue_uplink(uplink_type_t uplink_type)
{
	uint8_t device_tx_quiet;
	device_tx_quiet = get_device_tx_quiet();
	if(device_tx_quiet) {
		return;
	}

	uint32_t time_epochsec2020 = get_rtc_time();
		
	//State change uplinks have high priority, so always queue them. Only queue a status uplink if it has been more than 2 seconds since we last queued something.
	if (uplink_type == uplink_statechange)
	{
		queue_uplink(time_epochsec2020);
		last_uplink_queue_time_sec = time_epochsec2020;
	}
	else if (uplink_type == uplink_status)
	{
		if ((time_epochsec2020 - last_uplink_queue_time_sec) > 2)
		{
			queue_uplink(time_epochsec2020);
			last_uplink_queue_time_sec = time_epochsec2020;
		}
		else
		{
			LOG_DBG("status uplink dropped");
		}
	}
	
}

