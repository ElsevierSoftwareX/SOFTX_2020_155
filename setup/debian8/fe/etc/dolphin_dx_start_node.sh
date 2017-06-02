#!/bin/sh
# /etc/dolphin_dx_start_node.sh
#  - Start dolphin DX node on front-end

# count the number of adapters
# on Debian 8, lspci moved to /usr/bin
dx_cnt=`/usr/bin/lspci | grep Stargen | grep 0101 | wc -l`
echo "Found $dx_cnt DX adapters to configure"

dx_idx=1
dx_apt=0
while [ "$dx_idx" -le "$dx_cnt" ]
do

# Stop any existing node manager
  /etc/init.d/dis_nodemgr stop

# Set node to start an unused node ID (communication with nodemgr will set correctly)
  echo "configure DX card $dx_idx as adapter $dx_apt"
  /opt/DIS/sbin/dxconfig -c $dx_idx -a $dx_apt -slw 4 -n 4
  dx_idx=$(($dx_idx+1))

# Start up Dolphin node manager
  /etc/init.d/dis_nodemgr start

# let it work
  sleep 5

done
