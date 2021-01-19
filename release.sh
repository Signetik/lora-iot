#!/bin/sh

BUILD_DIR=_build
HEX=${BUILD_DIR}/zephyr/zephyr.hex
#UPDATE=${BUILD_DIR}/zephyr/app_update.bin

pushd ../zephyr
GIT_Z_DIFF=`git diff --quiet --exit-code || echo +`
GIT_Z_TAG=`git describe --exact-match`
if [ $? -eq 0 ]; then
    GIT_Z_TAG=`git describe --exact-match`
else
    GIT_Z_TAG=`git rev-parse --short HEAD`
fi
popd

( cd client ; ./ver.sh )
GIT_DIFF=`git diff --quiet --exit-code || echo +`
GIT_TAG=`git describe --tags`

west build -b signetik_siglrn client -d ${BUILD_DIR}

#echo "BIN file is for updating from server"
echo "[${GIT_TAG}:${GIT_Z_TAG}]"
if [ ${GIT_TAG} == ${GIT_Z_TAG} ] ; then
    vername=${GIT_TAG}${GIT_DIFF}${GIT_Z_DIFF}
else
    vername=${GIT_TAG}${GIT_DIFF}-z${GIT_Z_TAG}${GIT_Z_DIFF}
fi
cp ${HEX} siglrn_${vername}.hex
echo "HEX file siglrn_${vername} is for flashing locally"
#cp ${UPDATE} siglrn_${GIT_TAG}${GIT_DIFF}.bin
