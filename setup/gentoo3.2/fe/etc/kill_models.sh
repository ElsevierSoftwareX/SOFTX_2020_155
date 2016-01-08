#!/bin/bash

# To kill things, we 
# - kill non-IOP models
# - kill IOP model
#   Keith Thorne May, 2013

# Now we get the lists for this machine
HOSTNAME=`hostname`
SYSTEMS=`/etc/rt.sh`
IOPMOD=`/etc/rt.sh | awk '{print $1}'`
MODULES=`/etc/rt.sh | sed s/${IOPMOD}//`

for femod in $MODULES
do
   echo kill model $femod
   sudo -u controls /etc/run_stdenv.sh kill${femod}
done
#
echo kill IOP model $IOPMOD
sudo -u controls /etc/run_stdenv.sh kill${IOPMOD}

