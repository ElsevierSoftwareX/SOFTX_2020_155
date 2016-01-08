#!/bin/sh
#  /etc/dolphin_dx_config.sh
# return 0 exit status if in dolphin DX node table
if /etc/dolphin_dx_present.sh 
then
   HOSTNAME=`hostname`
   grep -v \^# /etc/dxnodetab | grep $HOSTNAME  >& /dev/null
else
   exit
fi
