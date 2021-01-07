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

