/*============================================================================*
 *         Copyright © 2019-2021 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                          SPDX-License-Identifier: Sigentik *
 *                                                                            *
 * Customer may modify, compile, assemble and convert this source code into   *
 * binary object or executable code for use on Signetk products purchased     *
 * from Signetik or its distributors.                                         *
 *                                                                            *
 * Customer may incorporate or embed an binary executable version of the      *
 * software into the Customer’s product(s), which incorporate a Signetik      *
 * product purchased from Signetik or its distributors. Customer may          *
 * manufacture, brand and distribute such Customer’s product(s) worldwide to  *
 * its End-Users.                                                             *
 *                                                                            *
 * This agreement must be formalized with Signetik before Customer enters     *
 * production and/or distributes products to Customer's End-Users             *
 *============================================================================*/

#define	LORA_STACKSIZE		2048
#define	LORA_PRIORITY		   4

#define	UART_STACKSIZE		1024
#define	UART_PRIORITY		   3

#define	BT_STACKSIZE		1024
#define	BT_PRIORITY			   2

#define	LED_STACKSIZE		 512
#define	LED_PRIORITY		   3

#define	GPS_STACKSIZE		1024
#define	GPS_PRIORITY		   3

#define	MAIN_STACKSIZE		 512
#define	MAIN_PRIORITY		   5

#define	MODBUS_STACKSIZE	 512
#define	MODBUS_PRIORITY		   6

#define	SIG_CONF_STACKSIZE	 512
#define	SIG_CONF_PRIORITY	   7

#define	WDT_STACKSIZE		 512
#define	WDT_PRIORITY		   8

#define	THREAD_START_DELAY	 500 //	ms
