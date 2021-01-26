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
 * utility.c
 *
 * Created: 8/21/2020 10:48:14 AM
 *  Author: Remy Patterson
 */ 

#include <stdio.h>
#include <stdarg.h>
#include <zephyr.h>
#include "main.h"
#include "utility.h"
#include <time.h>

#ifdef STEVE
void epochsec2020_to_calendar_time(uint32_t epochsecs_2020, struct rtc_calendar_time* time)
{
	//Convert to Unix epoch seconds
	const uint32_t unix_epoch_start_to_2020_sec = 1577836800;
	time_t unix_epoch_sec = epochsecs_2020 + unix_epoch_start_to_2020_sec;
	
	//Convert to time struct
	struct tm time_struct;
	memcpy(&time_struct, gmtime(&unix_epoch_sec), sizeof (struct tm));
	
	//put in RTC calendar struct
	time->year = time_struct.tm_year + 1900;
	time->month = time_struct.tm_mon + 1;
	time->day = time_struct.tm_mday;
	time->hour = time_struct.tm_hour;
	time->minute = time_struct.tm_min;
	time->second = time_struct.tm_sec;

}


//This function sets the system RTC, given seconds since 12:00:00AM January 1, 2020 UTC
void set_rtc(uint32_t epochsecs_2020)
{
	struct rtc_calendar_time time;
	
	epochsec2020_to_calendar_time(epochsecs_2020, &time);
	
	//set the RTC
	rtc_calendar_set_time(&rtc_instance, &time);
}
#endif

//This function returns the current RTC time, in seconds since 12:00:00AM January 1, 2020 UTC
uint32_t get_rtc_time()
{
#ifdef STEVE
	//Get RTC calendar time
	struct rtc_calendar_time time;
	rtc_calendar_get_time(&rtc_instance, &time);
	
	//Convert to time struct
	struct tm t;
	t.tm_year = time.year - 1900;
	t.tm_mon = time.month - 1;
	t.tm_mday = time.day;
	t.tm_hour = time.hour;
	t.tm_min = time.minute;
	t.tm_sec = time.second;
	
	//Convert to unix epoch seconds
	time_t unix_epoch_sec = mktime(&t);
	
	const uint32_t unix_epoch_start_to_2020_sec = 1577836800;
	uint32_t epoch_secs_2020 = unix_epoch_sec - unix_epoch_start_to_2020_sec;
	
	return epoch_secs_2020;
#else
	return 0;
#endif
}


uint16_t crc16_veridart(char *data, unsigned int len)
{
    /*
	printf("size of timing profile: %d\r\n", len);
    
    printf("timing profile contents:\r\n");
    for(int i = 0; i < len; i++)
    {
	    printf("0x%02x\r\n", data[i]);
    }
	*/
	uint16_t remainder = 0xFFFF;
    uint16_t polynomial = 0x1021;
    for( unsigned int i = 0; i < len; i++ ) {
        remainder ^= data[i] << 8;
        for( uint8_t bit = 8; bit > 0; bit--) {
            if( (remainder & 0x8000) )
                remainder = (remainder << 1) ^ polynomial;
            else
                remainder <<= 1;
        }
    }
	//printf("timing profile CRC is: %x\r\n", remainder);
    return remainder;
}
