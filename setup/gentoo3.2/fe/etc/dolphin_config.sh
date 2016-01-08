#!/bin/sh
#  /etc/dolphin_config.sh
# return 0 exit status if in dolphin node table
if /etc/dolphin_present.sh 
then
   HOSTNAME=`hostname`
   grep -v \^# /etc/dxnodetab | grep $HOSTNAME  >& /dev/null
else
   exit
fi
