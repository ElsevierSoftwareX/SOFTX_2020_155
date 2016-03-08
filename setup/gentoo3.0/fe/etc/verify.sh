#!/bin/bash

# Verify front ends are running

HOSTNAME=`hostname`
MODULES=`/etc/rt.sh`

for i in $MODULES
do
echo module $i
grep gps /proc/$i || exit 1
gps1=`grep gps /proc/$i| sed 's/gps=\(.*\)/\1/g'`
sleep 2
gps2=`grep gps /proc/$i| sed 's/gps=\(.*\)/\1/g'`
test $gps2 -gt $gps1 || (echo  ERROR: $i is not running; exit 1)
done

