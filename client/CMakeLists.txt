# /*============================================================================*
#  *         Copyright © 2019-2021 Signetik, LLC -- All Rights Reserved         *
#  *----------------------------------------------------------------------------*
#  *                                                              Signetik, LLC *
#  *                                                           www.signetik.com *
#  *                                          SPDX-License-Identifier: Sigentik *
#  *                                                                            *
#  * Customer may modify, compile, assemble and convert this source code into   *
#  * binary object or executable code for use on Signetk products purchased     *
#  * from Signetik or its distributors.                                         *
#  *                                                                            *
#  * Customer may incorporate or embed an binary executable version of the      *
#  * software into the Customer’s product(s), which incorporate a Signetik      *
#  * product purchased from Signetik or its distributors. Customer may          *
#  * manufacture, brand and distribute such Customer’s product(s) worldwide to  *
#  * its End-Users.                                                             *
#  *                                                                            *
#  * This agreement must be formalized with Signetik before Customer enters     *
#  * production and/or distributes products to Customer's End-Users             *
#  *============================================================================*/

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(client)


set(sources
	src/main.c
	src/uart_task.c
	src/vars.c
	src/wdt_task.c
	src/LoRa_task.c
	src/led_task.c
	src/bt_task.c
	src/gatt_config_service.c
)

# Main app
target_sources(app PRIVATE ${sources})
target_sources(app PRIVATE _sigver.c)

target_sources_ifdef(CONFIG_SIGNETIK_APP_LORATEST app PRIVATE src/loratest_task.c)
