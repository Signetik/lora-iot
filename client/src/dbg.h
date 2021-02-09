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

#pragma once
#include <stdio.h>

// #define DEBUG 1
 #undef  DEBUG

#define DBG_NEWLINE "\n"

#define INFO(...)   printf(__VA_ARGS__); printf(DBG_NEWLINE);
#define INFOX(...); printf(__VA_ARGS__);
#define ERR(...)    printf(__VA_ARGS__); printf(DBG_NEWLINE);

#ifdef DEBUG
	#define DBG(...)   fprintf(stderr,"%s:%d ",__FILE__,__LINE__); fprintf(stderr,__VA_ARGS__); fprintf(stderr,"\r\n");
	#define DBGX(...)  fprintf(stderr,__VA_ARGS__);
	#define DBGLX(...) fprintf(stderr,"%s:%d ",__FILE__,__LINE__); fprintf(stderr,__VA_ARGS__);
	#define DBG_PDU(x) CoapPDU_printBin(x);
#else
	#define DBG(...)   {};
	#define DBGX(...)  {};
	#define DBGLX(...) {};
	#define DBG_PDU(x) {};
#endif
