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
 * utility.h
 *
 * Created: 8/21/2020 10:48:43 AM
 *  Author: Remy Patterson
 */ 


void set_rtc(uint32_t epochsecs_2020);

uint32_t get_rtc_time();

uint16_t crc16_veridart(char *data_p, unsigned int length);


