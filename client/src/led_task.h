/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#ifndef	__LED_TASK_H
#define	__LED_TASK_H

typedef	struct
{
	bool enable;
	bool red;	
	bool green;
	bool blue;
} led_msg_t;

extern struct k_msgq led_msgq;

void led_thread_start(void);
#endif /* __LED_TASK_H */
