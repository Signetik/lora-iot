#!/bin/sh

BUILD_DIR=_build
HEX=${BUILD_DIR}/zephyr/zephyr.hex
#UPDATE=${BUILD_DIR}/zephyr/app_update.bin

( cd client ; ./ver.sh )
GIT_DIFF=`git diff --quiet --exit-code || echo +`
GIT_TAG=`git describe --tags`

west build -b signetik_siglrn client -d ${BUILD_DIR}

echo "HEX file is for flashing locally"
#echo "BIN file is for updating from server"
cp ${HEX} siglrn_${GIT_TAG}${GIT_DIFF}.hex
#cp ${UPDATE} siglrn_${GIT_TAG}${GIT_DIFF}.bin
