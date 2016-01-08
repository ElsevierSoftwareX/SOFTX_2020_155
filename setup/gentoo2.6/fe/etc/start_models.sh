#!/bin/bash

# To start things, we 
# - start IOP model
# - start non-IOP models
#   Keith Thorne May, 2013

# Now we get the lists for this machine
HOSTNAME=`hostname`
SYSTEMS=`/etc/rt.sh`
IOPMOD=`/etc/rt.sh | awk '{print $1}'`
MODULES=`/etc/rt.sh | sed s/${IOPMOD}//`
#
echo start IOP model $IOPMOD
sudo -u controls /etc/run_stdenv.sh start${IOPMOD}

for femod in $MODULES
do
   echo start model $femod
   sudo -u controls /etc/run_stdenv.sh start${femod}
done

