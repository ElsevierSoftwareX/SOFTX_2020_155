#!/bin/sh

# - Archive old daqd log files, if needed
#
#  First parameter - logdir
logdir=$1

# archive old log files, creating archive if needed
if [ ! -d ${logdir} ]; then
   mkdir ${logdir}
fi
cd ${logdir}
if [ ! -d archive ]; then
   echo "create archive directory"
   mkdir archive
fi
# make sure there is something to copy
loglist=`ls daqd.log*`
if [ ! -z "$loglist" ]; then
  for oldlog in daqd.log*; do
   mv -f "$oldlog" archive
  done
fi
exit 0

