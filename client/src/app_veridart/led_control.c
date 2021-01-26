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
 * led_control.c
 *
 * Created: 8/20/2020 10:48:20 AM
 *  Author: Remy Patterson
 */ 

#include <asf.h>
#include <stdint.h>
#include "led_control.h"

#define LED_PIN    PIN_PB03 


void led_on()
{
	ioport_set_pin_level(LED_PIN, IOPORT_PIN_LEVEL_HIGH);
}

void led_off()
{
	ioport_set_pin_level(LED_PIN, IOPORT_PIN_LEVEL_LOW);
}

void led_init()
{
	ioport_set_pin_dir(LED_PIN,IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(LED_PIN, IOPORT_PIN_LEVEL_LOW);
}

