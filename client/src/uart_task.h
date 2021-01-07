/*============================================================================*
 *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                      SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#ifndef __UART_TASK_H
#define __UART_TASK_H

void uart_thread_start(void);
int  uart_send(uint8_t *buffer, int length);

#endif /* __UART_TASK_H */
