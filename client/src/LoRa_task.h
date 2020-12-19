/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#ifndef	__LORA_TASK_H
#define	__LORA_TASK_H

//#include "cell_packet.h"

extern uint8_t * COAP_buff_ptr;
extern uint32_t	 COAP_buff_len;
extern int64_t base_time;

void lora_thread_start(void);

//extern const cell_packet_def_t *sig_default_packet_def;

#endif /* __LTE_TASK_H */
