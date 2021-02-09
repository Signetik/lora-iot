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

#ifndef __VERSION__H
#define __VERSION__H

extern const char* GIT_TAG;
extern const char* GIT_REV;
extern const char* GIT_BRANCH;

#define VERSION_MAJOR (GIT_TAG[1] - '0' + 0)
#define VERSION_MINOR (GIT_TAG[3] - '0' + 0)
#define VERSION_INCREMENTAL (GIT_TAG[5] - '0' + 0)
#define VERSION_ENGG (0)

#endif /* __VERSION__H */
