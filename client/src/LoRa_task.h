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

#ifndef	__LORA_TASK_H
#define	__LORA_TASK_H

#include "vars.h"

void lora_thread_start(void);
int	lora_push(char *key, char *value);

extern struct k_msgq lora_tx_queue;

#define	LORA_QUEUE_OBJECT_SIZE 53

struct lora_tx_message {
	uint32_t length;
	uint8_t	message[LORA_QUEUE_OBJECT_SIZE];
};


#endif /* __LORA_TASK_H	*/
