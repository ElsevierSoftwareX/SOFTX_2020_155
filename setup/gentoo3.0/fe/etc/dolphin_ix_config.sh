#!/bin/sh
#  /etc/dolphin_ix_config.sh
# return 0 exit status if in dolphin IX node table
if /etc/dolphin_ix_present.sh 
then
   HOSTNAME=`hostname`
   grep -v \^# /etc/ixnodetab | grep $HOSTNAME  >& /dev/null
else
   exit
fi
