# /*============================================================================*
#  *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
#  *----------------------------------------------------------------------------*
#  *                                                              Signetik, LLC *
#  *                                                           www.signetik.com *
#  *                                      SPDX-License-Identifier: BSD-4-Clause *
#  *============================================================================*/

#cmake_minimum_required(VERSION 3.8.2)
cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(send)


set(sources
	src/main.c
)

# Main app
target_sources(app PRIVATE ${sources})

