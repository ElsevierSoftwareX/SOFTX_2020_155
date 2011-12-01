#!/usr/bin/env bash
#  script to build new mbuf
if [ $# -gt 0 ]; then
    RCGTAG=$1
    echo " Build new mbuf for RCG $RCGTAG"
else
    echo " You need to add RCGTAG as input parameter"
    exit
fi
#   
source /opt/cdscfg/rtsetup.sh

cd ${RCG_DIR}
cd src/drv/mbuf
make
cd /lib/modules/2.6.34.1/kernel/drivers/mbuf
sudo bash -c "cp -p mbuf.ko mbuf.ko.old"
sudo bash -c "cp ${RCG_DIR}/src/drv/mbuf/mbuf.ko mbuf.ko.${RCGTAG}"
sudo bash -c "lsmod | grep mbuf && rmmod mbuf"
sudo bash -c "cp -p mbuf.ko.${RCGTAG} mbuf.ko"
sudo bash -c "insmod mbuf.ko"

