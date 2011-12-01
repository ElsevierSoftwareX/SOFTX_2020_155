#!/usr/bin/env bash
#  script to build new awgtpman
if [ $# -gt 0 ]; then
    RCGTAG=$1
    echo " Build new awgtpman for RCG $RCGTAG"
else
    echo " You need to add RCGTAG as input parameter"
    exit
fi
#
source /opt/cdscfg/rtsetup.sh
#
cd ${RCG_DIR}/src/gds
make clean
make
cp -p awgtpman ${RTCDSROOT}/target/gds/bin/awgtpman-${RCGTAG}
cd ${RTCDSROOT}/target/gds/bin
mkdir -p bin_archive
mv awgtpman-${RCGTAG} bin_archive
cp -p awgtpman bin_archive/awgtpman-pre${RCGTAG}
cp -p bin_archive/awgtpman-${RCGTAG} awgtpman
