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
#include <zephyr/types.h>
#include "main.h"
#include "utility.h"
#include <time.h>

static int64_t rtc_offset = 0;

//This function sets the system RTC, given seconds since 12:00:00AM January 1, 2020 UTC
void set_rtc(uint32_t epochsecs_2020)
{
	int64_t uptime = k_uptime_get();

	rtc_offset = epochsecs_2020 - (uptime / 1000);
}

//This function returns the current RTC time, in seconds since 12:00:00AM January 1, 2020 UTC
uint32_t get_rtc_time()
{
	int64_t uptime = k_uptime_get();

	return (uptime / 1000) + rtc_offset;
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
