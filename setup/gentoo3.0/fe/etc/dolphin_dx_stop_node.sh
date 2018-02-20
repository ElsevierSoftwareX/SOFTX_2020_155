#!/bin/sh
# /etc/dolphin_dx_stop_node.sh
#  - Start dolphin DX nodes on front-end

# Stop any existing node manager
/etc/init.d/dis_dx_nodemgr stop

# count the number of adapters
dx_cnt=`/usr/sbin/lspci | grep Stargen | grep 0101 | wc -l`
echo "Found $dx_cnt DX adapters to configure"

# dxtool starts from 0, not one ...
dx_idx=0
dx_apt=0
while [ "$dx_idx" -lt "$dx_cnt" ]
do
# prepare to shutdown node
  /opt/DIS/sbin/dxtool prepare-shutdown $dx_idx

# let it work
  sleep 5

# increment
  dx_idx=$(($dx_idx+1))

done
