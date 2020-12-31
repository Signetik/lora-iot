/*============================================================================*
 *		   Copyright ©	2019-2020 Signetik,	LLC	-- All Rights Reserved		   *
 *----------------------------------------------------------------------------*
 *																Signetik, LLC *
 *															 www.signetik.com *
 *										SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#ifndef	__LORA_TASK_H
#define	__LORA_TASK_H

#include "vars.h"

void lora_thread_start(void);
int	lora_push(char *key, char *value);


//extern const cell_packet_def_t *sig_default_packet_def;

#endif /* __LTE_TASK_H */
