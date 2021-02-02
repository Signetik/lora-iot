# /*============================================================================*
#  *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
#  *----------------------------------------------------------------------------*
#  *                                                              Signetik, LLC *
#  *                                                           www.signetik.com *
#  *                                      SPDX-License-Identifier: BSD-4-Clause *
#  *============================================================================*/

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(client)


set(sources
	src/main.c
	src/uart_task.c
	src/vars.c
	src/wdt_task.c
	src/sigconfig.c
#	src/cbor.c
#	src/coap.c
#	src/cantcoap.c
#	src/sysdep.c
	src/LoRa_task.c
	src/led_task.c
	src/bt_task.c
	src/gatt_config_service.c
	src/modem_binary_interface_api.c
	src/modem_binary_interface_api_device.c
	src/modem_ascii_interface_api.c
)

# Main app
target_sources(app PRIVATE ${sources})
target_sources(app PRIVATE _sigver.c)
