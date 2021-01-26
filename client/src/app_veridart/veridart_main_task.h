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
 * veridart_main_task.h
 *
 * Created: 8/19/2020 10:42:41 AM
 *  Author: Remy Patterson
 */ 

#ifndef __VERIDART_MAIN_TASK_H
#define __VERIDART_MAIN_TASK_H

#include <stdint.h>
#include <zephyr.h>

#define VERIDART_STACKSIZE   512
#define VERIDART_PRIORITY	   8

void set_device_state(uint8_t state);
uint8_t get_device_state(void);
void veridart_main_task(void* p);


void set_device_tx_quiet(uint8_t state);
uint8_t get_device_tx_quiet(void);

#endif /* __VERIDART_MAIN_TASK_H */
