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

#!/bin/sh

GIT_DIFF=`git diff --quiet --exit-code || echo +`
GIT_TAG=`git describe --tags`
GIT_BRANCH=`git rev-parse --abbrev-ref HEAD`

FILE=_sigver.c

echo "/* THIS FILE IS GENERATED FROM ver.sh */" > ${FILE}
echo "/* DO NOT COMMIT IT OFTEN or EVER. */" >> ${FILE}
echo "const char* GIT_REV=\"${GIT_REV}${GIT_DIFF}\";" >> ${FILE}
echo "const char* GIT_TAG=\"${GIT_TAG}${GIT_DIFF}$1\";" >> ${FILE}
echo "const char* GIT_BRANCH=\"${GIT_BRANCH}\";" >> ${FILE}

