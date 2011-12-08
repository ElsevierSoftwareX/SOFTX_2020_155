#!/usr/bin/env bash
#  script to build new daqd
if [ "$# -gt 0" ]; then
    RCGTAG=$1
    echo " Build new mbuf for RCG $RCGTAG"
else
    echo " You need to add RCGTAG as input parameter"
    exit
fi
#   
source /opt/cdscfg/rtsetup.sh

cd ${RTBUILD_DIR}
#
make standiop
cp -p build/standiop/daqd ${RTCDSROOT}/target/fb/daqd-${RCGTAG}
cd  ${RTCDSROOT}/target/fb
mkdir -p bin_archive
mv daqd-${RCGTAG} bin_archive
cp -p daqd bin_archive/daqd-pre${RCGTAG}
cp -p bin_archive/daqd-${RCGTAG} daqd
