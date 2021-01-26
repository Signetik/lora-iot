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
//   Signetik Confidential Proprietary Information -- Disclosure Prohibited   //
//----------------------------------------------------------------------------//
// Project   : SafeTraces veriDART                                            //
// Filename  :                                                                //
// Author(s) :                                                                //
// Created   :                                                                //
// Purpose   :                                                                //
//============================================================================//
// NOTE: Signetik will transfer ownership to SafeTraces on final delivery.


#ifndef __VERIDART_MESSAGES_H
#define __VERIDART_MESSAGES_H

#include <stdint.h>

typedef enum {
	uplink_status,
	uplink_statechange
} uplink_type_t;


void veridart_process_downlink(uint8_t* packet, uint8_t packetLen);
void veridart_queue_uplink(uplink_type_t uplink_type);
void rtc_match_callback(void);



typedef struct timingprofile_s {
	uint16_t pre_delay_sec;
	uint32_t on_msec;    //
	uint16_t number_of_sprays;  // 1 for Sampler
	uint16_t off_sec;           // unused for Sampler
} timingprofile_t;

typedef struct operation_data_s {
	uint8_t relay_active; // 1: relay is ON, 0: relay is OFF
	uint16_t completed_iterations; // current count of completed sprays (or samples. For sampler it will be 1 if done)
} operation_data_t;


enum operation_complete_result {
	success = 0,
	error = 1,
	canceled = 2
};

enum device_state {
	device_booted = 1, // device just powered up
	device_timesetup, // device received time from server
	device_obtained_timing_profile, // device received timing profile from server
	device_obtained_go, // device obtained go and will start at the said time
	device_start_delay, // Operation started, Device inside delay_before Spray/Sample window
	device_active_start, // started a spray (or sample) == relay activated
	device_active_end, // finsihed a spray (or sample)  == relay inactivated
	device_operation_complete, // Running operation per Timing profile is complete
	device_operation_stopped, // An active operation was stopped
};

/* status message sent by device has different details based on the current state of the device */
enum pkt_type {
	pkt_device_status_booted = 100,
	pkt_device_timesetup,
	pkt_device_obtained_timing_profile,
	pkt_device_obtained_go,
	pkt_device_start_delay,
	pkt_device_active_start,
	pkt_device_active_end,
	pkt_device_operation_complete,
	pkt_device_operation_stopped,


	pkt_server_bootupACK = 200,
	pkt_server_setclock,
	pkt_server_timesetupACK,
	pkt_server_settimingprofile,
	pkt_server_timingprofileACK,
	pkt_server_go,
	pkt_server_goACK,
	pkt_server_completedACK,
	pkt_server_stop,
};

typedef struct device_status_s {
	uint8_t pkt_type;
	uint8_t device_state; // = enum device_state
	uint32_t epochsecs_2020; // clock time in secs since 2020-01-01
	uint16_t crc16_tp_data; // crc16 hash of "timingprofile_t tp_data"
	operation_data_t op_data;
	uint8_t op_result; // enum operation_complete_result
} device_status_t;


typedef struct device_status_booted_s {
	uint8_t pkt_type; // = pkt_device_status_booted
	uint8_t device_state; // = enum device_state
	
} device_status_booted_t;


typedef struct device_status_timesetup_s {
	uint8_t pkt_type; // = pkt_device_timesetup
	uint8_t device_state; // = enum device_state
	uint32_t epochsecs_2020; // clock time in secs since 2020-01-01
	
} device_status_timesetup_t;


typedef struct device_status_obtained_timing_profile_s {
	uint8_t pkt_type; // = pkt_device_obtained_timing_profile
	uint8_t device_state; // = enum device_state
	uint32_t epochsecs_2020; // clock time in secs since 2020-01-01

	timingprofile_t tp; // a copy of what is on device so server can check if it is in sync
	
} device_status_obtained_timing_profile_t;



typedef struct device_status_start_delay_s {
	uint8_t pkt_type; // = pkt_device_start_delay
	uint8_t device_state; // = enum device_state
	uint32_t epochsecs_2020; // clock time in secs since 2020-01-01

	timingprofile_t tp_data; // a copy of what is on device so server can check if it is in sync
	operation_data_t op_data; //
} device_status_start_delay_t;

typedef struct device_status_active_start_s {
	uint8_t pkt_type; // = pkt_device_active_start
	uint8_t device_state; // = enum device_state
	uint32_t epochsecs_2020; // clock time in secs since 2020-01-01

	//NOTE: TBD if we should send tp_data
	timingprofile_t tp_data; // a copy of what is on device so server can check if it is in sync
	operation_data_t op_data; // .relay_active=1
} device_status_active_start_t;

typedef struct device_status_active_end_s {
	uint8_t pkt_type; // = pkt_device_active_end
	uint8_t device_state; // = enum device_state
	uint32_t epochsecs_2020; // clock time in secs since 2020-01-01

	//NOTE: TBD if we should send tp_data
	timingprofile_t tp_data; // a copy of what is on device so server can check if it is in sync
	operation_data_t op_data; // .relay_active=0
} device_status_active_end_t;

typedef struct device_status_operation_complete_s {
	uint8_t pkt_type; // = pkt_device_operation_complete
	uint8_t device_state; // = enum device_state
	uint32_t epochsecs_2020; // clock time in secs since 2020-01-01

	timingprofile_t tp_data; // a copy of what is on device so server can check if it is in sync
	operation_data_t op_data; // .relay_active=0
	uint8_t op_result; // enum operation_complete_result
} device_status_operation_complete_t;

typedef struct device_status_operation_stopped_s {
        uint8_t pkt_type; // = pkt_device_operation_stopped
        uint8_t device_state; // = enum device_state
        uint32_t epochsecs_2020; // clock time in secs since 2020-01-01

        timingprofile_t tp_data; // a copy of what is on device so server can check if it is in sync
        operation_data_t op_data; // .relay_active=0
        uint8_t op_result; // enum operation_complete_result
} device_status_operation_stopped_t;


typedef struct server_status_timesetup_s {
	uint8_t pkt_type; // = pkt_server_setclock
	uint32_t epochsecs_2020; // clock time in secs since 2020-01-01
	
} server_status_timesetup_t;

//NOTE: serer will compute start_time
typedef struct server_status_setprofile_s {
	uint8_t pkt_type; // = pkt_server_settimingprofile
	timingprofile_t tp_data; // Timing profile data
} server_status_setprofile_t;

//Note: When Project Start button is pressed, Server computes NOW+1minute and sends epoch_secs_2020 as start-of-operation-time
// 1minute buffer is added to allow all devices to receive this message
// Based on testing, we might have to adjust this buffer time, and hardcode (or config) the LoRa-txtime-buffer-secs on the server
typedef struct server_status_go_s {
	uint8_t pkt_type; // = pkt_server_go
	uint32_t start_time_epochsecs_2020; // time at which operation should start
} server_status_go_t;

// Note: Stop message would stop an operation in progress.
// It has no effect if operation is not in progress
// Another Go command would start the operation afresh. 
// There is no Pause/Resume option.
typedef struct server_status_stop_s {
        uint8_t pkt_type; // = pkt_server_stop
} server_status_stop_t;


#endif /* __VERIDART_MESSAGES_H */

