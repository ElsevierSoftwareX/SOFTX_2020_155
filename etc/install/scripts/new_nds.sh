#!/bin/bash
# script to build new stand-alone NDS
if [ $# -gt 0 ]; then
    RCGTAG=$1
    echo " Build new mbuf for RCG $RCGTAG"
else
    echo " You need to add RCGTAG as input parameter"
    exit
fi
#
source /opt/cdscfg/rtsetup.sh
#
cdsCode
make nds
cp -p build/nds/nds ${RTCDSROOT}/target/fb/nds-${RCGTAG}
cd ${RTCDSROOT}/target/fb
mkdir -p bin_archive
mv nds-${RCGTAG} bin_archive
cp -p nds bin_archive/nds-pre${RCGTAG}
cp -p bin_archive/nds-${RCGTAG} nds


