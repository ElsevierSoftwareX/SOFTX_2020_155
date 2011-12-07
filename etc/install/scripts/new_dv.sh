#!/usr/bin/env bash
#  script to build new dataviewer
if [ $# -gt 0 ]; then
    RCGTAG=$1
    echo " Build new dataviewer for RCG $RCGTAG"
else
    echo " You need to add RCGTAG as input parameter"
    exit
fi
#
source /opt/cdscfg/rtsetup.sh
#
cd /opt/rtapps
if [ -L dv ]; then
   rm dv
elif [ -d dv ]; then
   mv dv dv-pre${RCGTAG}
fi
#
cd ${RCG_DIR}/src/dv
make clean
make
make install
cd /opt/rtapps
mv dv dv-${RCGTAG}
ln -s dv-${RCGTAG} dv

