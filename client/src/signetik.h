/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#define	LORA_STACKSIZE		1024
#define	LORA_PRIORITY		   2

#define	UART_STACKSIZE		1024
#define	UART_PRIORITY		   2

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
