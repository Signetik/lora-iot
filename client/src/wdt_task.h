/*============================================================================*
 *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                      SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#ifndef __WDT_TASK_H
#define __WDT_TASK_H

void    wdt_thread_start(void);
uint8_t wdt_register_thread(void);
void    wdt_deregister_thread(uint8_t thread_id);
void    wdt_feed_watchdog(uint8_t thread_id);
uint32_t wdt_get_timeout_val(void);

#endif /* __WDT_TASK_H */
